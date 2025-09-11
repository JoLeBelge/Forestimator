/*package alt

import (
	"fmt"
	"log"
	"net"
	"strconv"
	"strings"
	"time"
)

func main() {
	s := NetworkPasstroughSettings{
		bufferSize:   1024 * 1024 * 10,
		retryCount:   50,
		toHost:       "localhost:8001",
		fromPort:     "8085",
		retryTimeout: 50 * time.Millisecond,
		readTimeout:  100 * time.Microsecond,
		writeTimeout: 5 * time.Second,
	}
	proxyLoop(s)
}

type NetworkPasstroughSettings struct {
	bufferSize   int
	retryCount   int
	retryTimeout time.Duration
	readTimeout  time.Duration
	writeTimeout time.Duration
	toHost       string
	fromPort     string
}

func parseRequest(request string, startState int, contentLength int) (restRequest string, endState int, restContentLength int) {
	switch startState {
	case 0:
		if strings.Contains(request, "Content-Length:") {
			contentLength := strings.Split(request, "Content-Length: ")[1]
			parts := strings.Split(contentLength, "\r")
			contentLength = parts[0]
			number, err := strconv.Atoi(contentLength)
			if err != nil {
				log.Printf("Error parsing Content-Length: %v", err)
				return "", startState, -1
			}
			return parseRequest(request, startState+1, number)
		} else {
			return "", startState, -1
		}
	case 1:
		if strings.Contains(request, "\r\n\r\n") {
			parts := strings.Split(request, "\r\n\r\n")
			restRequest = parts[1]
			return parseRequest(restRequest, startState+1, contentLength)
		} else {
			return restRequest, startState, contentLength
		}
	case 2:
		switch true {
		case len(request) == contentLength:
			return "", -1, 0
		case 0 == contentLength:
			return "", -1, 0
		case len(request) < contentLength:
			return "", startState, contentLength - len(request)
		default:
			return request, 666, contentLength
		}
	default:
		return "", -2, 0
	}
}

func requestFinished(request string) bool {
	defer func() {
		if err := recover(); err != nil {
			log.Println("Error during parsing: ", err)
			log.Println(request)
		}
	}()
	state := 0
	_, state, _ = parseRequest(request, 0, 0)
	return state == -1
}

func handleConnection(conn net.Conn, s NetworkPasstroughSettings) {
	buf := make([]byte, s.bufferSize)
	hostRequest := make([]byte, s.bufferSize)
	hostResponse := make([]byte, s.bufferSize)
	host, err := net.Dial("tcp", s.toHost)
	if err != nil {
		log.Printf("Failed to connect to port 6665: %v", err)
		return
	}
	lenRequest := 0

	retryReceiving := s.retryCount
	for retryReceiving > 0 {
		loopUser := 0
		for loopUser < 5 {
			conn.SetReadDeadline(time.Now().Add(s.readTimeout))
			n, err := conn.Read(buf)
			if err != nil {
				loopUser++
			} else {
				loopUser = 0
				hostRequest = append(hostRequest[:lenRequest], buf[:n]...)
				lenRequest += n
				buf = buf[n:]
				if requestFinished(string(hostRequest[:lenRequest])) {
					retryReceiving = 0
				} else {
					retryReceiving = 2
				}
			}
		}
		if retryReceiving > 2 {
			time.Sleep(s.retryTimeout)
		}
		retryReceiving--
	}

	if lenRequest > 0 {
		conn.SetWriteDeadline(time.Now().Add(s.writeTimeout))
		_, err = host.Write(hostRequest[:lenRequest])
		if err != nil {
			log.Printf("Failed to send data to port 6665: %v", err)
		}
		lenResponse := 0

		retryReceiving = s.retryCount
		for retryReceiving > 0 {
			loopForest := 0
			for loopForest < 5 {
				host.SetReadDeadline(time.Now().Add(s.readTimeout))
				n, err := host.Read(buf)
				if err != nil {
					loopForest++
				} else {
					loopForest = 0
					hostResponse = append(hostResponse[:lenResponse], buf[:n]...)
					lenResponse += n
					buf = buf[n:]
					if requestFinished(string(hostResponse[:lenResponse])) {
						retryReceiving = 0
					} else {
						retryReceiving = 2
					}
				}
			}
			if retryReceiving > 2 {
				time.Sleep(s.retryTimeout)
			}
			retryReceiving--
		}
		if lenResponse > 0 {
			conn.SetWriteDeadline(time.Now().Add(s.writeTimeout))
			_, err = conn.Write(hostResponse[:lenResponse])
			if err != nil {
				log.Printf("Failed to send data to port %s: %v", s.fromPort, err)
			}
		} else {
			log.Println("Timeout! forestimator Web did not respond")
		}
	} else {
		log.Println("Timeout! Empty request received")
	}
	conn.Close()
	host.Close()
	fmt.Println("Connection closed")
}

func proxyLoop(s NetworkPasstroughSettings) {
	addr, err := net.ResolveTCPAddr("tcp", ":"+s.fromPort)
	if err != nil {
		log.Fatal(err)
	}
	ln, err := net.ListenTCP("tcp", addr)
	if err != nil {
		log.Fatal(err)
	}
	defer ln.Close()
	fmt.Println("Listening on port", s.fromPort)
	for {
		conn, err := ln.Accept()
		if err != nil {
			log.Fatal(err)
		}
		go handleConnection(conn, s)
		fmt.Println("Accepted new connection")
	}
}
*/