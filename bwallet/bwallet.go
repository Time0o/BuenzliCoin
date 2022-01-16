package main

import (
	"bufio"
	"bytes"
	"crypto/ecdsa"
	"crypto/elliptic"
	"crypto/rand"
	"crypto/sha256"
	"crypto/x509"
	"encoding/base64"
	"encoding/hex"
	"encoding/json"
	"encoding/pem"
	"errors"
	"flag"
	"fmt"
	"io/ioutil"
	"net/http"
	"os"
	"path/filepath"
	"regexp"
	"strconv"
	"strings"
)

type wallet struct {
	Name    string
	Key     string
	Address string
}

type transactionInput struct {
	OutputHash  string `json:"output_hash"`
	OutputIndex int    `json:"output_index"`
	Signature   string `json:"signature"`
}

type transactionOutput struct {
	Amount  int    `json:"amount"`
	Address string `json:"address"`
}

type transactionUnspentOutput struct {
	OutputHash  string            `json:"output_hash"`
	OutputIndex int               `json:"output_index"`
	Output      transactionOutput `json:"output"`
}

type transaction struct {
	Type    string              `json:"type"`
	Index   int                 `json:"index"`
	Hash    string              `json:"hash"`
	Inputs  []transactionInput  `json:"inputs"`
	Outputs []transactionOutput `json:"outputs"`
}

func getBuenzliDir() (string, error) {
	buenzliDir := os.Getenv("BUENZLI_DIR")
	if buenzliDir == "" {
		homeDir, err := os.UserHomeDir()
		if err != nil {
			return "", err
		}

		buenzliDir = filepath.Join(homeDir, ".buenzli")
	}

	_, err := os.Stat(buenzliDir)
	if err != nil {
		if os.IsNotExist(err) {
			err := os.Mkdir(buenzliDir, 0755)
			if err != nil {
				return "", err
			}
		} else {
			return "", err
		}
	}

	return buenzliDir, nil
}

func getBuenzliNode() (string, error) {
	buenzliNode := os.Getenv("BUENZLI_NODE")
	if buenzliNode == "" {
		return "", errors.New("BUENZLI_NODE not set")
	}

	return buenzliNode, nil
}

func getWallets(buenzliDir string) ([]wallet, error) {
	wallets := make([]wallet, 0)

	// Open wallets file.
	walletsFilePath := filepath.Join(buenzliDir, "wallets")

	walletsFile, err := os.OpenFile(walletsFilePath, os.O_CREATE|os.O_RDONLY, 0644)
	if err != nil {
		return nil, err
	}
	defer walletsFile.Close()

	// Parse wallets.
	walletRe := regexp.MustCompile("([[:alnum:]]*) (.*) ([A-Za-z0-9+/=]*)")

	walletsScanner := bufio.NewScanner(walletsFile)
	for walletsScanner.Scan() {
		walletId := walletsScanner.Text()

		match := walletRe.FindStringSubmatch(walletId)
		if match == nil {
			return nil, errors.New(fmt.Sprintf("malformed wallet identifier '%s'", walletId))
		}

		w := wallet{
			Name:    match[1],
			Key:     filepath.Join(buenzliDir, match[2]),
			Address: match[3]}

		wallets = append(wallets, w)
	}

	return wallets, nil
}

func findWallet(buenzliDir string, name string) (*wallet, error) {
	ws, err := getWallets(buenzliDir)
	if err != nil {
		return nil, err
	}

	for _, w := range ws {
		if w.Name == name {
			return &w, nil
			break
		}
	}

	return nil, nil
}

func walletUnspentOutputs(
	buenzliNode string,
	w wallet) ([]transactionUnspentOutput, error) {

	resp, err := http.Get("http://" + buenzliNode + "/transactions/unspent")
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()

	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return nil, err
	}

	var unspentOutputs []transactionUnspentOutput

	if err := json.Unmarshal(body, &unspentOutputs); err != nil {
		return nil, err
	}

	var matchingUnspentOutputs []transactionUnspentOutput

	for _, unspentOutput := range unspentOutputs {
		output := unspentOutput.Output
		if output.Address == w.Address {
			matchingUnspentOutputs = append(matchingUnspentOutputs, unspentOutput)
		}
	}

	return matchingUnspentOutputs, nil
}

// XXX Encrypt private keys.
func createWallet(buenzliDir string, name string) error {
	// Check that key does not already exist.
	w, err := findWallet(buenzliDir, name)
	if err != nil {
		return err
	}

	if w != nil {
		return errors.New(fmt.Sprintf("wallet '%s' already exist", name))
	}

	// Generate key pair.
	key, err := ecdsa.GenerateKey(elliptic.P256(), rand.Reader)
	if err != nil {
		return err
	}

	// Store private key.
	keyFileName := "id_ecdsa_" + name
	keyFilePath := filepath.Join(buenzliDir, keyFileName)

	keyPemPrivate, err := os.Create(keyFilePath)
	if err != nil {
		return err
	}

	keyBytesPrivate, err := x509.MarshalECPrivateKey(key)
	if err != nil {
		return err
	}

	keyBlockPrivate := &pem.Block{
		Type:  "EC PRIVATE KEY",
		Bytes: keyBytesPrivate,
	}

	if err = pem.Encode(keyPemPrivate, keyBlockPrivate); err != nil {
		return err
	}

	// Get public key.
	keyBytesPublic, err := x509.MarshalPKIXPublicKey(&key.PublicKey)
	if err != nil {
		return err
	}

	keyStrPublic := strings.Trim(base64.StdEncoding.EncodeToString(keyBytesPublic), "=")

	// Add entry to wallets file.
	walletsFilePath := filepath.Join(buenzliDir, "wallets")

	walletsFile, err := os.OpenFile(walletsFilePath, os.O_CREATE|os.O_WRONLY|os.O_APPEND, 0644)
	if err != nil {
		return err
	}

	walletStr := fmt.Sprintf("%s %s %s\n", name, keyFileName, keyStrPublic)

	if _, err := walletsFile.WriteString(walletStr); err != nil {
		return err
	}

	return nil
}

func mineIntoWallet(buenzliDir string, buenzliNode string, name string) error {
	w, err := findWallet(buenzliDir, name)
	if err != nil {
		return err
	}

	if w == nil {
		return errors.New(fmt.Sprintf("wallet '%s' does not exist", name))
	}

	addressJson, _ := json.Marshal(w.Address)

	resp, err := http.Post(
		"http://"+buenzliNode+"/blocks",
		"application/json",
		bytes.NewBuffer(addressJson))
	if err != nil {
		return err
	}
	defer resp.Body.Close()

	return nil
}

func walletBalance(buenzliDir string, buenzliNode string, of string) (int, error) {
	w, err := findWallet(buenzliDir, of)
	if err != nil {
		return -1, err
	}

	if w == nil {
		return -1, errors.New(fmt.Sprintf("wallet '%s' does not exist", of))
	}

	unspentOutputs, err := walletUnspentOutputs(buenzliNode, *w)
	if err != nil {
		return -1, err
	}

	balance := 0

	for _, unspentOutput := range unspentOutputs {
		output := unspentOutput.Output

		balance += output.Amount
	}

	return balance, nil
}

func walletTransfer(
	buenzliDir string,
	buenzliNode string,
	from string,
	to string,
	amount int) error {

	w, err := findWallet(buenzliDir, from)
	if err != nil {
		return err
	}

	if w == nil {
		return errors.New(fmt.Sprintf("wallet '%s' does not exist", from))
	}

	unspentOutputs, err := walletUnspentOutputs(buenzliNode, *w)
	if err != nil {
		return err
	}

	// Construct transaction.
	trans := transaction{
		Type:    "standard",
		Index:   0, // XXX Correct next index (add endpoint).
		Inputs:  []transactionInput{},
		Outputs: []transactionOutput{}}

	total := 0

	for _, unspentOutput := range unspentOutputs {
		input := transactionInput{
			unspentOutput.OutputHash,
			unspentOutput.OutputIndex,
			""}

		trans.Inputs = append(trans.Inputs, input)

		output := unspentOutput.Output

		total += output.Amount

		if total >= amount {
			break
		}
	}

	if total < amount {
		return errors.New("insufficient funds")
	}

	outputSend := transactionOutput{Amount: amount, Address: to}
	trans.Outputs = append(trans.Outputs, outputSend)

	if total > amount {
		outputReturn := transactionOutput{Amount: total - amount, Address: w.Address}
		trans.Outputs = append(trans.Outputs, outputReturn)
	}

	// Hash transaction.
	ss := ""

	ss += strconv.Itoa(trans.Index)

	for _, txi := range trans.Inputs {
		ss += txi.OutputHash
		ss += strconv.Itoa(txi.OutputIndex)
	}

	for _, txo := range trans.Outputs {
		ss += strconv.Itoa(txo.Amount)
		ss += txo.Address
	}

	hash := sha256.Sum256([]byte(ss))

	trans.Hash = hex.EncodeToString(hash[:])

	// Sign transaction inputs.
	keyStr, err := ioutil.ReadFile(w.Key)
	if err != nil {
		return err
	}

	keyBlock, _ := pem.Decode(keyStr)

	key, err := x509.ParseECPrivateKey(keyBlock.Bytes)
	if err != nil {
		return err
	}

	signature, err := ecdsa.SignASN1(rand.Reader, key, hash[:])
	if err != nil {
		return err
	}

	for i := range trans.Inputs {
		trans.Inputs[i].Signature = hex.EncodeToString(signature)
	}

	// Post transaction.
	transJson, _ := json.Marshal(trans)

	resp, err := http.Post(
		"http://"+buenzliNode+"/transactions",
		"application/json",
		bytes.NewBuffer(transJson))
	if err != nil {
		return err
	}
	defer resp.Body.Close()

	return nil
}

func success() int {
	return 0
}

func failure(e error) int {
	fmt.Fprintf(os.Stderr, "error: %v\n", e)
	return 1
}

func run() int {
	// Locate .buenzli directory.
	buenzliDir, err := getBuenzliDir()
	if err != nil {
		return failure(err)
	}

	// Locate buenzli node.
	buenzliNode, err := getBuenzliNode()
	if err != nil {
		return failure(err)
	}

	// Parse command line arguments.
	listWalletsCmd :=
		flag.NewFlagSet("list", flag.ExitOnError)

	createWalletCmd :=
		flag.NewFlagSet("create", flag.ExitOnError)
	createWalletCmdName :=
		createWalletCmd.String("name", "", "wallet name")

	mineIntoWalletCmd :=
		flag.NewFlagSet("mine", flag.ExitOnError)
	mineIntoWalletCmdTo :=
		mineIntoWalletCmd.String("to", "", "wallet name")

	walletBalanceCmd :=
		flag.NewFlagSet("balance", flag.ExitOnError)
	walletBalanceCmdOf :=
		walletBalanceCmd.String("of", "", "wallet name")

	walletTransferCmd :=
		flag.NewFlagSet("balance", flag.ExitOnError)
	walletTransferCmdFrom :=
		walletTransferCmd.String("from", "", "name of source wallet")
	walletTransferCmdTo :=
		walletTransferCmd.String("to", "", "address of target wallet")
	walletTransferCmdAmount :=
		walletTransferCmd.Int("amount", 0, "number of coins to send")

	if len(os.Args) < 2 {
		return failure(errors.New("expected subcommand (list|create|mine|balance|transfer)"))
	}

	subcommand := os.Args[1]
	subcommandArgs := os.Args[2:]

	switch subcommand {
	case "list":
		listWalletsCmd.Parse(subcommandArgs)

		ws, err := getWallets(buenzliDir)
		if err != nil {
			return failure(err)
		}

		for _, w := range ws {
			fmt.Printf("%s: %s\n", w.Name, w.Address)
		}
	case "create":
		createWalletCmd.Parse(subcommandArgs)

		if *createWalletCmdName == "" {
			return failure(errors.New("-name argument is required"))
		}

		if err := createWallet(buenzliDir, *createWalletCmdName); err != nil {
			return failure(err)
		}
	case "mine":
		mineIntoWalletCmd.Parse(subcommandArgs)

		if *mineIntoWalletCmdTo == "" {
			return failure(errors.New("-to argument is required"))
		}

		if err := mineIntoWallet(buenzliDir, buenzliNode, *mineIntoWalletCmdTo); err != nil {
			return failure(err)
		}
	case "balance":
		walletBalanceCmd.Parse(subcommandArgs)

		if *walletBalanceCmdOf == "" {
			return failure(errors.New("-of argument is required"))
		}

		balance, err := walletBalance(buenzliDir, buenzliNode, *walletBalanceCmdOf)
		if err != nil {
			return failure(err)
		}

		fmt.Println(balance)
	case "transfer":
		walletTransferCmd.Parse(subcommandArgs)

		if *walletTransferCmdFrom == "" {
			return failure(errors.New("-from argument is required"))
		}

		if *walletTransferCmdTo == "" {
			return failure(errors.New("-to argument is required"))
		}

		if *walletTransferCmdAmount <= 0 {
			return failure(errors.New("-amount argument must be positive"))
		}

		err := walletTransfer(buenzliDir,
			buenzliNode,
			*walletTransferCmdFrom,
			*walletTransferCmdTo,
			*walletTransferCmdAmount)
		if err != nil {
			return failure(err)
		}
	default:
		return failure(errors.New(fmt.Sprintf("unknown subcommand '%s'", subcommand)))
	}

	return success()
}

func main() {
	os.Exit(run())
}
