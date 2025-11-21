package main

import (
	"errors"
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

type proc struct {
	proxy            *httputil.ReverseProxy
	downloader       *httputil.ReverseProxy
	listOfProperties string
	updateintervall  time.Duration
	started          bool
	cmd              *exec.Cmd
	outPipe          io.ReadCloser
	errPipe          io.ReadCloser
	starttime        time.Time
	infoDirName      string
	bufferSize       int

	pid             int
	state           string
	vmPeak          int
	vmSize          int
	vmLck           int
	vmPin           int
	vmHwM           int
	vmRSS           int
	vmData          int
	vmStk           int
	vmExe           int
	vmLib           int
	vmPTE           int
	vmSwap          int
	hugetlbPages    int
	coreDumping     int
	thpEnabled      int
	threads         int
	sigQ            string
	sigPnd          string
	shdPnd          string
	sigBlk          string
	sigIgn          string
	sigCgt          string
	capInh          string
	capPrm          string
	capEff          string
	capBnd          string
	capAmb          string
	cpusAllowedList string
}

func startForestimatorWebServer() (cmd *exec.Cmd) {
	cmd = exec.Command("sh", "launch.sh") //, "--deploy-path=/ --docroot \"/home/carto/app/Forestimator/data/;/favicon.ico,/google52ee6b8ebe0b4b19.html,/sitemap.xml,/resources,/style,/tmp,/data,/js,/jslib,/img,/pdf,/video,resources/themes/bootstrap/5\" --http-port 8001 --http-addr 127.0.0.1 -c /home/gef/Documents/Forestimator/data/wt_config.xml --BD \"/home/gef/Documents/Forestimator/carteApt/data/aptitudeEssDB.db\" --colPath \"Dir\" ")
	cmd.Dir = "/home/gef/Documents/Forestimator/forestimatorWeb"
	return cmd
}

func startForestimatorDownloadServer() (cmd *exec.Cmd) {
	cmd = exec.Command("/usr/local/go/bin/go", "run", "downloadServer.go") //, "--deploy-path=/ --docroot \"/home/carto/app/Forestimator/data/;/favicon.ico,/google52ee6b8ebe0b4b19.html,/sitemap.xml,/resources,/style,/tmp,/data,/js,/jslib,/img,/pdf,/video,resources/themes/bootstrap/5\" --http-port 8001 --http-addr 127.0.0.1 -c /home/gef/Documents/Forestimator/data/wt_config.xml --BD \"/home/gef/Documents/Forestimator/carteApt/data/aptitudeEssDB.db\" --colPath \"Dir\" ")
	cmd.Dir = "/home/gef/Documents/Forestimator/forestimatorDownloadServer"
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

func parseProc(p *proc, keylist string) (map[string]string, error) {
	cmd := exec.Command("cat", "status")
	cmd.Dir = "/proc/" + strconv.Itoa(p.pid)
	output, err := cmd.Output()
	if err != nil {
		log.Printf("Error parsing proc: %v", err)
		return nil, err
	}

	lines := strings.Split(string(output), "\n")
	result := make(map[string]string)

	for _, line := range lines {
		if strings.Contains(line, ":") {
			parts := strings.SplitN(line, ":", 2)
			if len(parts) == 2 {
				key := strings.TrimSpace(parts[0])
				value := strings.TrimSpace(parts[1])
				if keylist == "" || strings.Contains(keylist, key) {
					result[key] = value
				}
			}
		}
	}
	return result, nil
}

func refreshProcInfo(p *proc) (info map[string]string, err error) {
	info, err = parseProc(p, p.listOfProperties)
	if err != nil {
		log.Printf("Error getting status: %v", err)
		return nil, err
	}
	p.state = info["State"]
	n, _ := strconv.Atoi(info["Threads"])
	p.threads = n
	n, _ = strconv.Atoi(strings.Split(info["VmPeak"], " ")[0])
	p.vmPeak = n
	n, _ = strconv.Atoi(strings.Split(info["VmSize"], " ")[0])
	p.vmSize = n
	n, _ = strconv.Atoi(strings.Split(info["VmHWM"], " ")[0])
	p.vmHwM = n
	n, _ = strconv.Atoi(strings.Split(info["VmData"], " ")[0])
	p.vmData = n
	n, _ = strconv.Atoi(strings.Split(info["CoreDumping"], " ")[0])
	p.coreDumping = n
	p.sigQ = info["SigQ"]
	p.sigPnd = info["SigPnd"]
	p.sigBlk = info["SigBlk"]
	p.sigIgn = info["SigIgn"]
	p.sigCgt = info["SigCgt"]
	return info, nil
}

func recordData(p *proc, data map[string]string) {
	path := p.infoDirName + "/" + "statusRecord.csv"
	newfile := false
	if _, err := os.Stat(path); errors.Is(err, os.ErrNotExist) {
		newfile = true
	}
	openFile, _ := os.OpenFile(path, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
	if newfile {
		openFile.Write([]byte(strings.ReplaceAll(p.listOfProperties, ": ", ",")))
		openFile.Write([]byte("\n"))
	}
	openFile.Write([]byte(strings.Split(time.Now().String(), "+")[0]))
	openFile.Write([]byte(","))
	for _, property := range strings.Split(p.listOfProperties, ": ") {
		openFile.Write([]byte(data[property]))
		openFile.Write([]byte(","))
	}
	openFile.Write([]byte("\n"))
	openFile.Close()
}

func recordRequest(p *proc, request string) {
	path := p.infoDirName + "/" + "requestRecord.csv"
	newfile := false
	if _, err := os.Stat(path); errors.Is(err, os.ErrNotExist) {
		newfile = true
	}
	openFile, _ := os.OpenFile(path, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
	if newfile {
		openFile.Write([]byte("date, request"))
		openFile.Write([]byte("\n"))
	}
	openFile.Write([]byte(strings.Split(time.Now().String(), "+")[0]))
	openFile.Write([]byte(","))
	openFile.Write([]byte(request))
	openFile.Write([]byte("\n"))
	openFile.Close()
}

func recordTerminalLogs(p *proc) {
	path := p.infoDirName + "/" + "termOutRecord.csv"
	newfile := false
	if _, err := os.Stat(path); errors.Is(err, os.ErrNotExist) {
		newfile = true
	}
	openFile, _ := os.OpenFile(path, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
	if newfile {
		openFile.Write([]byte("date, log"))
		openFile.Write([]byte("\n"))
	}
	text := make([]byte, p.bufferSize)
	t, _ := p.outPipe.Read(text)
	openFile.Write([]byte(strings.Split(time.Now().String(), "+")[0]))
	openFile.Write([]byte(",\"\"\""))
	openFile.Write(text[:t])
	openFile.Write([]byte("\"\"\"\n"))
	openFile.Close()
}
func recordErrorLogs(p *proc) {
	path := p.infoDirName + "/" + "termErrRecord.csv"
	newfile := false
	if _, err := os.Stat(path); errors.Is(err, os.ErrNotExist) {
		newfile = true
	}
	openFile, _ := os.OpenFile(path, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
	if newfile {
		openFile.Write([]byte("date, log"))
		openFile.Write([]byte("\n"))
	}
	text := make([]byte, p.bufferSize)
	t, _ := p.errPipe.Read(text)
	openFile.Write([]byte(strings.Split(time.Now().String(), "+")[0]))
	openFile.Write([]byte(",\"\"\""))
	openFile.Write([]byte(text[:t]))
	openFile.Write([]byte("\"\"\"\n"))
	openFile.Close()
}

func recordEndLog(p *proc) {
	path := p.infoDirName + "/" + "endLog.txt"
	openFile, _ := os.OpenFile(path, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
	openFile.Write([]byte(time.Now().String()))
	openFile.Write([]byte("\n"))
	cmd := exec.Command("cat", "status")
	cmd.Dir = "/proc/" + strconv.Itoa(p.pid)
	output, _ := cmd.Output()
	openFile.Write([]byte(output))
	openFile.Write([]byte("\n"))
	openFile.Write([]byte(strconv.Itoa(p.cmd.ProcessState.ExitCode())))
	openFile.Write([]byte("\n"))
	openFile.Close()
}

func recordThisInFile(printtime bool, this string, filename string, path string) {
	openFile, _ := os.OpenFile(path+"/"+filename, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
	if printtime {
		openFile.Write([]byte(time.Now().String()))
		openFile.Write([]byte("\n"))
	}
	openFile.Write([]byte(this))
	openFile.Write([]byte("\n"))
	openFile.Close()
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

func getProcessInfo(p *proc) string {
	cmd := exec.Command("ls", "/proc/"+strconv.Itoa(p.pid)+"/task")
	out, _ := cmd.Output()
	output := strings.Split(string(out), "\n")
	var states string
	printState := false
	for _, thread := range output[:len(output)-1] {
		cmd = exec.Command("cat", "stat")
		cmd.Dir = "/proc/" + strconv.Itoa(p.pid) + "/task/" + thread
		out, _ = cmd.Output()
		output := strings.Split(string(out), " ")
		if len(output) >= 3 {
			states += output[2]
			if output[2] != "S" {
				printState = true
			}
		} else {
			states += "^"
			printState = true
		}
	}

	var value []string
	cmd = exec.Command("cat", "status")
	cmd.Dir = "/proc/" + strconv.Itoa(p.pid)
	out, _ = cmd.Output()
	sigOut := strings.Split(string(out), "SigIgn:")
	if len(sigOut) >= 2 {
		value = strings.Split(sigOut[1], "\n")
	}

	if printState {
		if len(value) >= 1 {
			return states + " - " + strings.TrimSpace(value[0])
		} else {
			return states + " - ?"
		}
	}
	return ""
}

func main() {
	forestimator := proc{
		listOfProperties: "Name: State: VmPeak: VmSize: VmLck: VmPin: VmHWM: VmRSS: VmData: VmStk: VmExe: VmLib: VmPTE: VmSwap: HugetlbPages: CoreDumping: ThpEnabled: Threads: SigQ: SigPnd: ShdPnd: SigBlk: SigIgn: SigCgt: CapInh: CapPrm: CapEff: CapBnd: CapAmb: Cpus_allowed_list:",
		updateintervall:  500 * time.Millisecond,
		started:          false,
		cmd:              nil,
		outPipe:          nil,
		pid:              -1,
		bufferSize:       1 << 20,
	}

	go func() {
		for {
			forestimator.cmd = startForestimatorWebServer()
			log.Println(forestimator.cmd.Args)
			forestimator.outPipe, _ = forestimator.cmd.StdoutPipe()
			forestimator.errPipe, _ = forestimator.cmd.StderrPipe()
			err := forestimator.cmd.Start()
			if err == nil {
				forestimator.starttime = time.Now()
				log.Println(forestimator.cmd.Args)
				go func() {
					time.Sleep(100 * time.Millisecond)
					forestimator.pid = getForestimatorPid("forestimatorWeb")
					t := strconv.Itoa(time.Now().Year()) + "." +
						time.Now().Month().String() + "." +
						strconv.Itoa(time.Now().Day()) + "." +
						strconv.Itoa(time.Now().Hour()) + "." + strconv.Itoa(time.Now().Minute()) + "." + strconv.Itoa(time.Now().Second()) + "."
					forestimator.infoDirName = "logs/" + t + strconv.Itoa(forestimator.pid)
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
			recordEndLog(&forestimator)
			log.Println("Forestimator web server has stopped")
			log.Println("Restarting now...")
		}
	}()
	go func() {
		for {
			cmd := startForestimatorDownloadServer()
			cmd.Start()
			cmd.Wait()
			log.Println("Forestimator download server has stopped")
			log.Println("Restarting now...")
		}

	}()
	go func() {
		forestimator.proxy, _ = NewProxy("http://localhost:8500")
		forestimator.downloader, _ = NewProxy("http://localhost:8501")
		forestimator.openforis, _ = NewProxy("http://localhost:8080")
		for {
			http.Handle("/collect", forestimator.openforis)
			if forestimator.started {
				director := forestimator.proxy.Director
				forestimator.proxy.Director = func(r *http.Request) {
					director(r)
					recordRequest(&forestimator, r.RequestURI)
				}
				http.Handle("/", forestimator.proxy)
				http.Handle("/results/", forestimator.downloader)
				log.Println(http.ListenAndServe(":8085", nil))
				log.Println("Proxy server has stopped")
				log.Println("Restarting now...")
			}
		}
	}()
	go func() {
		timeup := true
		for {
			if forestimator.started && !timeup {
				record, _ := refreshProcInfo(&forestimator)
				recordData(&forestimator, record)
				timeup = true
			} else {
				time.Sleep(forestimator.updateintervall)
				timeup = false
			}
		}
	}()
	go func() {
		timeup := true
		tmp := []string{}
		for {
			if forestimator.started && !timeup {
				processInfo := getProcessInfo(&forestimator)
				if processInfo != "" {
					tmp = append(tmp, strings.TrimSpace(processInfo))
				}
				timeup = true
			} else {
				if len(tmp) > 10 {
					for _, str := range tmp {
						if str != "" {
							recordThisInFile(false, strings.Split(time.Now().String(), "+")[0]+","+str+","+strconv.Itoa(forestimator.vmPeak), "threadSSig.csv", forestimator.infoDirName)
						}
					}
					tmp = []string{}
				} else {
					time.Sleep(time.Nanosecond * 100)
				}
				timeup = false
			}
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
	go func() {
		timeup := true
		for {
			if forestimator.started && !timeup {
				recordTerminalLogs(&forestimator)
				timeup = true
			} else {
				time.Sleep(time.Millisecond * 100)
				timeup = false
			}
		}
	}()
	go func() {
		timeup := true
		for {
			if forestimator.started && !timeup {
				recordErrorLogs(&forestimator)
				timeup = true
			} else {
				time.Sleep(time.Millisecond * 10)
				timeup = false
			}
		}
	}()
	select {}
}
