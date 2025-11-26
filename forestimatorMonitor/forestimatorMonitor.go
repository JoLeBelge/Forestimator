package main

import (
	"bytes"
	"fmt"
	"log"
	"net/http"
	"os/exec"
	"time"
	"net/smtp"
)

const (
	serverURL   = "https://forestimator.gembloux.ulg.ac.be/"
	pingIP      = "139.165.226.56"
	emailTo     = "tthissen@uliege.be"
	maxPingTime = 10 * time.Second
)

type RequestLog struct {
	Response string
	Time     time.Time
}

func main() {
	for {
		// Attempt the request
		resp, err := http.Get(serverURL)
		if err != nil {
			log.Printf("Request failed: %v\n", err)
			handleFailure()
		} else {
			log.Println("Request succeeded, sleeping for 1 hour...")
			time.Sleep(1 * time.Hour)
			resp.Body.Close()
		}
	}
}

func handleFailure() {
	var logs [3]RequestLog
	var pingLog bytes.Buffer

	// Retry loop
	for i := 0; i < 3; i++ {
		time.Sleep(15 * time.Minute)

		resp, err := http.Get(serverURL)
		if err != nil {
			logs[i] = RequestLog{Response: fmt.Sprintf("Attempt %d failed: %v", i+1, err), Time: time.Now()}
			log.Printf("Attempt %d failed: %v\n", i+1, err)
		} else {
			log.Printf("Attempt %d succeeded, resuming normal operation.\n", i+1)
			time.Sleep(1 * time.Hour)
			resp.Body.Close()
			return
		}
	}

	// If all retries failed, attempt a ping
	pingCmd := exec.Command("ping", "-c", "4", "-W", "10", pingIP)
	pingCmd.Stdout = &pingLog
	pingCmd.Stderr = &pingLog
	pingErr := pingCmd.Run()

	if pingErr != nil {
		log.Printf("Ping failed: %v\n", pingErr)
		sendErrorReport(logs, pingLog.String())
	} else {
		log.Println("Ping succeeded, resuming normal operation.")
		time.Sleep(1 * time.Hour)
	}
}

func sendErrorReport(logs [3]RequestLog, pingLog string) {
	subject := "Server Monitoring Alert"
	body := fmt.Sprintf("The following requests failed:\n\n")
	for i, log := range logs {
		body += fmt.Sprintf("Attempt %d: %s at %s\n", i+1, log.Response, log.Time.Format(time.RFC3339))
	}
	body += fmt.Sprintf("\nPing log:\n%s\n", pingLog)

	// In a real-world scenario, you would use an email package to send the email.
	// For simplicity, we'll just print the report here.
	log.Printf("Sending error report to %s:\n\n%s\n", emailTo, body)
	// Uncomment the following lines to actually send an email (requires an SMTP setup)
		err := smtp.SendMail("smtp.ulg.ac.be:25",
			smtp.PlainAuth("", "u241781", "ap[Ko)19", "smtp.ulg.ac.be"),
			"panne@forestimator.gembloux.ulg.ac.be", []string{emailTo}, []byte(body))
		if err != nil {
			log.Printf("Failed to send email: %v\n", err)
		}

}
