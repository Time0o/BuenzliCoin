package main

import (
	"bufio"
	"bytes"
	"crypto/ecdsa"
	"crypto/elliptic"
	"crypto/rand"
	"crypto/x509"
	"encoding/base64"
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
	"strings"
)

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

type wallet struct {
	Name    string
	Address string
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

		wallets = append(wallets, wallet{Name: match[1], Address: match[3]})
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

	// Generate private key.
	key, err := ecdsa.GenerateKey(elliptic.P256(), rand.Reader)
	if err != nil {
		return err
	}

	keyFileName := "id_ecdsa_" + name
	keyFilePath := filepath.Join(buenzliDir, keyFileName)

	keyPem, err := os.Create(keyFilePath)
	if err != nil {
		return err
	}

	keyBytes, err := x509.MarshalECPrivateKey(key)
	if err != nil {
		return err
	}

	keyStr := strings.Trim(base64.StdEncoding.EncodeToString(keyBytes), "=")

	keyBlock := &pem.Block{
		Type:  "EC PRIVATE KEY",
		Bytes: keyBytes,
	}

	if err = pem.Encode(keyPem, keyBlock); err != nil {
		return err
	}

	// Add entry to wallets file.
	walletsFilePath := filepath.Join(buenzliDir, "wallets")

	walletsFile, err := os.OpenFile(walletsFilePath, os.O_CREATE|os.O_WRONLY|os.O_APPEND, 0644)
	if err != nil {
		return err
	}

	walletStr := fmt.Sprintf("%s %s %s\n", name, keyFileName, keyStr)

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

	resp, err := http.Post("http://"+buenzliNode+"/blocks",
		"application/json",
		bytes.NewBuffer(addressJson))
	if err != nil {
		return err
	}
	defer resp.Body.Close()

	return nil
}

type transactionOutput struct {
	Amount  int
	Address string
}

type transactionUnspentOutput struct {
	OutputHash  string
	OutputIndex int
	Output      transactionOutput
}

func walletBalance(buenzliDir string, buenzliNode string, name string) (int, error) {
	w, err := findWallet(buenzliDir, name)
	if err != nil {
		return -1, err
	}

	if w == nil {
		return -1, errors.New(fmt.Sprintf("wallet '%s' does not exist", name))
	}

	resp, err := http.Get("http://" + buenzliNode + "/transactions/unspent")
	if err != nil {
		return -1, err
	}
	defer resp.Body.Close()

	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return -1, err
	}

	var unspentOutputs []transactionUnspentOutput

	if err := json.Unmarshal(body, &unspentOutputs); err != nil {
		return -1, err
	}

	balance := 0

	for _, unspentOutput := range unspentOutputs {
		output := unspentOutput.Output
		if output.Address == w.Address {
			balance += output.Amount
		}
	}

	return balance, nil
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

	if len(os.Args) < 2 {
		return failure(errors.New("expected subcommand (list|create|mine|balance)"))
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
	default:
		return failure(errors.New(fmt.Sprintf("unknown subcommand '%s'", subcommand)))
	}

	return success()
}

func main() {
	os.Exit(run())
}
