package main

import (
	"io"
	"log"
	"net/http"
	"net/http/httputil"
	"net/url"
	"os"
	"os/exec"
	"strconv"
	"strings"
	"time"
)

const (
	portMiddleWare      = "47100"
	portForestimatorWeb = "8505"
	workingDirectory    = /*"/home/gef/Documents/" */ "/home/carto/app/"
)

type proc struct {
	debugMode             bool
	proxy                 *httputil.ReverseProxy
	downloader            *httputil.ReverseProxy
	openforis             *httputil.ReverseProxy
	started               bool
	cmd                   *exec.Cmd
	outPipe               io.ReadCloser
	errPipe               io.ReadCloser
	starttime             time.Time
	infoDirName           string
	totalRequestsReceived int

	pid int
}

func startForestimatorWebServer(wd string) (cmd *exec.Cmd) {
	cmd = exec.Command("sh", "launchDevelopment.sh") //, "--deploy-path=/ --docroot \"/home/carto/app/Forestimator/data/;/favicon.ico,/google52ee6b8ebe0b4b19.html,/sitemap.xml,/resources,/style,/tmp,/data,/js,/jslib,/img,/pdf,/video,resources/themes/bootstrap/5\" --http-port 8001 --http-addr 127.0.0.1 -c /home/gef/Documents/Forestimator/data/wt_config.xml --BD \"/home/gef/Documents/Forestimator/carteApt/data/aptitudeEssDB.db\" --colPath \"Dir\" ")
	cmd.Dir = wd + "Forestimator/forestimatorWeb"
	return cmd
}

func NewProxy(rawUrl string) (*httputil.ReverseProxy, error) {
	url, err := url.Parse(rawUrl)
	if err != nil {
		return nil, err
	}
	proxy := httputil.NewSingleHostReverseProxy(url)

	return proxy, nil
}

func getForestimatorPid(name string) int {
	cmd := exec.Command("pgrep", "-f", name)
	output, err := cmd.Output()
	if err != nil {
		log.Printf("Error getting PID: %v", err)
		return -1
	}
	pidStr := strings.TrimSpace(strings.Split(string(output), "\n")[0])
	pid, err := strconv.Atoi(pidStr)
	if err != nil {
		log.Printf("Error converting PID to int: %v", err)
		return -1
	}
	return pid
}

func sendRobotsTxt(w http.ResponseWriter, r *http.Request) {
	openFile, _ := os.Open(workingDirectory + "Forestimator/forestimatorMiddleware/robots.txt")
	defer openFile.Close()
	content, _ := io.ReadAll(openFile)
	w.Header().Set("Content-Type", "text/plain")
	w.Write([]byte(content))
}

func sendLLMsTxt(w http.ResponseWriter, r *http.Request) {
	openFile, _ := os.Open(workingDirectory + "Forestimator/forestimatorMiddleware/llms.txt")
	defer openFile.Close()
	content, _ := io.ReadAll(openFile)
	w.Header().Set("Content-Type", "text/plain")
	w.Write([]byte(content))
}

func main() {
	loop := true
	forestimator := proc{
		debugMode:             true,
		started:               false,
		cmd:                   nil,
		outPipe:               nil,
		pid:                   -1,
		totalRequestsReceived: 0,
	}

	go func() {

		for loop {
			forestimator.cmd = startForestimatorWebServer(workingDirectory)
			log.Println(forestimator.cmd.Args)
			forestimator.outPipe, _ = forestimator.cmd.StdoutPipe()
			forestimator.errPipe, _ = forestimator.cmd.StderrPipe()
			err := forestimator.cmd.Start()
			if err == nil {
				forestimator.starttime = time.Now()
				log.Println(forestimator.cmd.Args)
				go func() {
					time.Sleep(100 * time.Millisecond)
					forestimator.pid = getForestimatorPid("forestimatorWeb-dev")
					t := strconv.Itoa(time.Now().Year()) + "." +
						time.Now().Month().String() + "." +
						strconv.Itoa(time.Now().Day()) + "." +
						strconv.Itoa(time.Now().Hour()) + "." + strconv.Itoa(time.Now().Minute()) + "." + strconv.Itoa(time.Now().Second()) + "."
					forestimator.infoDirName = workingDirectory + "/Forestimator/forestimatorMiddleware/logs/" + t + strconv.Itoa(forestimator.pid)
					os.Mkdir(forestimator.infoDirName, os.ModePerm)
					forestimator.started = true
					log.Println("Started Forestimator web server. pid: " + strconv.Itoa(forestimator.pid))
				}()
			} else {
				log.Println("Failed to start Forestimator web server")
				log.Println(err)
			}
			forestimator.cmd.Wait()
			forestimator.started = false
			log.Println("Forestimator web server has stopped")
			if !forestimator.debugMode {
				log.Println("Restarting now...")
			} else {
				log.Println("Exiting now...")
				loop = false
			}
		}
	}()
	go func() {
		forestimator.proxy, _ = NewProxy("http://localhost:" + portForestimatorWeb)
		for {

			director := forestimator.proxy.Director
			forestimator.proxy.Director = func(r *http.Request) {
				director(r)
			}

			forestimatorHandler := http.NewServeMux()
			forestimatorHandler.Handle("/", forestimator.proxy)
			forestimatorHandler.HandleFunc("/robots.txt", sendRobotsTxt)
			forestimatorHandler.HandleFunc("/llms.txt", sendLLMsTxt)

			log.Println(http.ListenAndServe(":"+portMiddleWare, forestimatorHandler))

			log.Println("Proxy server has stopped")
			log.Println("Restarting now...")

		}
	}()

	go func() {
		timeup := true
		for timeup {
			if forestimator.started {
				timeup = false
			}
		}
	}()
	select {}
}
