package main

import (
	"bufio"
	"crypto/ecdsa"
	"crypto/elliptic"
	"crypto/rand"
	"crypto/x509"
	"encoding/base64"
	"encoding/pem"
	"errors"
	"flag"
	"fmt"
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

type wallet struct {
	name    string
	address string
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

		walletName := match[1]
		walletAddress := match[3]

		wallets = append(wallets, wallet{name: walletName, address: walletAddress})
	}

	return wallets, nil
}

// XXX Encrypt private keys.
func createWallet(buenzliDir string, walletName string) error {
	// Check that key does not already exist.
	ws, err := getWallets(buenzliDir)
	if err != nil {
		return err
	}

	exists := false
	for _, w := range ws {
		if w.name == walletName {
			exists = true
			break
		}
	}

	if exists {
		return errors.New(fmt.Sprintf("wallet '%s' already exist", walletName))
	}

	// Generate private key.
	key, err := ecdsa.GenerateKey(elliptic.P256(), rand.Reader)
	if err != nil {
		return err
	}

	keyFileName := "id_ecdsa_" + walletName
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

	walletStr := fmt.Sprintf("%s %s %s\n", walletName, keyFileName, keyStr)

	if _, err := walletsFile.WriteString(walletStr); err != nil {
		return err
	}

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

	// Parse command line arguments.
	listWalletsCmd :=
		flag.NewFlagSet("list_wallets", flag.ExitOnError)

	createWalletCmd :=
		flag.NewFlagSet("create_wallet", flag.ExitOnError)
	createWalletCmdName :=
		createWalletCmd.String("name", "", "wallet name")

	if len(os.Args) < 2 {
		return failure(errors.New("expected subcommand (list_wallets|create_wallet)"))
	}

	subcommand := os.Args[1]
	subcommandArgs := os.Args[2:]

	switch subcommand {
	case "list_wallets":
		listWalletsCmd.Parse(subcommandArgs)

		ws, err := getWallets(buenzliDir)
		if err != nil {
			return failure(err)
		}

		for _, w := range ws {
			fmt.Printf("%s: %s...%s\n", w.name, w.address[:15], w.address[len(w.address)-15:])
		}
	case "create_wallet":
		createWalletCmd.Parse(subcommandArgs)

		if *createWalletCmdName == "" {
			return failure(errors.New("-name argument is required"))
		}

		err := createWallet(buenzliDir, *createWalletCmdName)
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
