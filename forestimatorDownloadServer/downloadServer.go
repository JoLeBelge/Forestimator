// BTW: This is a standalone server program to serve files from a specific directory with cleanup of old files.
// Author: chatGPT
package main

import (
	"fmt"
	"html/template"
	"log"
	"net/http"
	"os"
	"path/filepath"
	"strings"
	"time"
)

const (
	downloadDir = "/home/carto/app/Forestimator/data/tmp"
	retention   = 7 * 24 * time.Hour // 1 week
)

// Simple HTML error template
var errorTemplate = template.Must(template.New("error").Parse(`
<!DOCTYPE html>
<html>
<head><title>Error</title></head>
<body>
  <h1>{{.Title}}</h1>
  <p>{{.Message}}</p>
</body>
</html>
`))

func main() {
	// Start cleanup routine
	go cleanupOldFiles()

	http.HandleFunc("/results/", handleDownload)

	addr := ":8501"
	log.Printf("Download server running on %s", addr)
	log.Fatal(http.ListenAndServe(addr, nil))
}

func handleDownload(w http.ResponseWriter, r *http.Request) {
	// Extract filename from URL
	filename := strings.TrimPrefix(r.URL.Path, "/results/")
	if filename == "" {
		renderError(w, http.StatusBadRequest, "Bad Request", "Filename not specified.")
		return
	}

	// Prevent directory traversal
	if strings.Contains(filename, "..") {
		renderError(w, http.StatusBadRequest, "Bad Request", "Invalid filename.")
		return
	}

	filePath := filepath.Join(downloadDir, filename)

	// Check if file exists
	info, err := os.Stat(filePath)
	if err != nil {
		if os.IsNotExist(err) {
			renderError(w, http.StatusNotFound, "Not Found", "The requested file does not exist.")
		} else {
			renderError(w, http.StatusInternalServerError, "Server Error", "Unable to access file.")
		}
		return
	}

	// Ensure it's not a directory
	if info.IsDir() {
		renderError(w, http.StatusBadRequest, "Bad Request", "Requested path is a directory.")
		return
	}

	// Serve file with download headers
	w.Header().Set("Content-Disposition", fmt.Sprintf("attachment; filename=%q", "AnalyseSurfaciqueForestimator.xml"))
	http.ServeFile(w, r, filePath)
}

func renderError(w http.ResponseWriter, status int, title, msg string) {
	w.WriteHeader(status)
	_ = errorTemplate.Execute(w, map[string]string{
		"Title":   title,
		"Message": msg,
	})
}

func cleanupOldFiles() {
	for {
		files, err := os.ReadDir(downloadDir)
		if err != nil {
			log.Printf("Error reading download dir: %v", err)
			time.Sleep(1 * time.Hour)
			continue
		}

		now := time.Now()
		for _, f := range files {
			path := filepath.Join(downloadDir, f.Name())
			info, err := f.Info()
			if err != nil {
				log.Printf("Error getting file info: %v", err)
				continue
			}
			if now.Sub(info.ModTime()) > retention {
				err := os.Remove(path)
				if err != nil {
					log.Printf("Failed to remove old file %s: %v", path, err)
				} else {
					log.Printf("Deleted old file: %s", path)
				}
			}
		}

		time.Sleep(1 * time.Hour) // check every hour
	}
}
