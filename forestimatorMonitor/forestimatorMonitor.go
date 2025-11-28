package main

import (
	"bytes"
	"fmt"
	"log"
	"net/http"
	"os/exec"
	"time"

	"github.com/wneessen/go-mail"
)

const (
	serverURL          = "https://forestimator.gembloux.ulg.ac.be/"
	pingIP             = "139.165.226.56"
	maxPingTime        = 10 * time.Second
	intervallRequest   = 1 * time.Hour
	intervallRetry     = 10 * time.Minute
	hoursNoHarrassment = 24
)

func emailTo() []string {
	return []string{"tthissen@uliege.be", "liseinjon@hotmail.com", "p.lejeune@uliege.be"}
}

type RequestLog struct {
	Response string
	Time     time.Time
}

func main() {
	failureTime := 0
	for {
		resp, err := http.Get(serverURL)
		if err != nil && resp.StatusCode != 200 {
			log.Printf("Request failed: %v\n", err)
			if failureTime < 1 {
				failureTime = handleFailure()
			} else {
				time.Sleep(intervallRequest)
				log.Println(failureTime)
				failureTime--
			}
		} else {
			log.Println("Request succeeded, sleeping for 1 hour...")
			if failureTime > 0 {
				failureTime = 0
				sendSuccessMailAfterFailure()
			}
			time.Sleep(intervallRequest)
			resp.Body.Close()
		}
	}
}

func handleFailure() int {
	var logs [3]RequestLog
	var pingLog bytes.Buffer

	// Retry loop
	for i := 0; i < 3; i++ {
		time.Sleep(intervallRetry)

		resp, err := http.Get(serverURL)
		if err != nil {
			logs[i] = RequestLog{Response: fmt.Sprintf("Attempt %d failed: %v", i+1, err), Time: time.Now()}
			log.Printf("Attempt %d failed: %v\n", i+1, err)
		} else {
			log.Printf("Attempt %d succeeded\n", i+1)
			resp.Body.Close()
			return 0
		}
	}

	// If all retries failed, attempt a ping
	pingCmd := exec.Command("ping", "-c", "4", "-W", "10", pingIP)
	pingCmd.Stdout = &pingLog
	pingCmd.Stderr = &pingLog
	pingErr := pingCmd.Run()

	if pingErr != nil {
		log.Printf("Ping failed: %v\n", pingErr)
		sendErrorReport(logs, pingLog.String(), false)
	} else {
		log.Println("Ping succeeded.")
		sendErrorReport(logs, pingLog.String(), true)
	}
	return hoursNoHarrassment
}

func sendErrorReport(logs [3]RequestLog, pingLog string, pingSuccess bool) {
	subject := "Forestimator Offline!"
	body := "The following requests failed:\n\n"
	for i, log := range logs {
		body += fmt.Sprintf("Attempt %d: %s at %s\n", i+1, log.Response, log.Time.Format(time.RFC3339)) + "\n"
	}
	if pingSuccess {
		body += "\nPing to server IP succeeded, indicating a possible server issue.\n"
	} else {
		body += "\nPing to server IP failed, indicating a possible network issue.\n"
	}
	body += fmt.Sprintf("\nPing log:\n%s\n", pingLog)
	for _, email := range emailTo() {
		SendEmail(email, subject, body)
	}
}

func sendSuccessMailAfterFailure() {
	subject := "Forestimator is back online!"
	body := "The Forestimator server is back online after previous failures.\n"
	for _, email := range emailTo() {
		SendEmail(email, subject, body)
	}
}

func SendEmail(recipient string, subject string, msg string) {

	// Sender data.
	m := mail.NewMsg()
	if err := m.From("tthissen@uliege.be"); err != nil {
		log.Printf("failed to set From address: %s", err)
	}
	if err := m.To(recipient); err != nil {
		log.Printf("failed to set To address: %s", err)
	}
	m.Subject(subject)
	m.SetBodyString(mail.TypeTextPlain, msg)

	// Secondly the mail client
	c, err := mail.NewClient("smtp.ulg.ac.be",
		mail.WithPort(465), mail.WithSMTPAuth(mail.SMTPAuthLogin),
		mail.WithUsername("tthissen@uliege.be"), mail.WithPassword("ap[Ko)19"),
		mail.WithSSLPort(false))
	if err != nil {
		log.Printf("failed to create mail client: %s", err)
	}

	// Finally let's send out the mail
	if err := c.DialAndSend(m); err != nil {
		log.Printf("failed to send mail: %s", err)
	}
}
