package main

import (
	"fmt"
	"os"
	"path/filepath"
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

func success() int {
	return 0
}

func failure(e error) int {
	fmt.Fprintf(os.Stderr, "error: %v\n", e)
	return 1
}

func run() int {
	buenzliDir, err := getBuenzliDir()
	if err != nil {
		return failure(err)
	}

	fmt.Println(buenzliDir) // XXX

	return success()
}

func main() {
	os.Exit(run())
}
