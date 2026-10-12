package main

import (
	"errors"
	"fmt"
	"io"
	"log"
	"net/http"
	"net/http/cgi"
	"net/http/httputil"
	"net/url"
	"os"
	"os/exec"
	"strconv"
	"strings"
	"time"
)

const (
	portMiddleWare             = "8085"
	portForestimatorWebOld     = "8500"
	portForestimatorWeb        = "8505"
	portForestimatorDownloader = "8501"
	portOpenForis              = "8380"
	bufferSize                 = 1 << 20
	updateIntervall            = 100 * time.Millisecond
	listOfProperties           = "Name: State: VmPeak: VmSize: VmLck: VmPin: VmHWM: VmRSS: VmData: VmStk: VmExe: VmLib: VmPTE: VmSwap: HugetlbPages: CoreDumping: ThpEnabled: Threads: SigQ: SigPnd: ShdPnd: SigBlk: SigIgn: SigCgt: CapInh: CapPrm: CapEff: CapBnd: CapAmb: Cpus_allowed_list:"
	workingDirectory           = /*"/home/gef/Documents/" */ "/home/carto/app/"
)

var mapMapservRoutes = map[string]string{
	"map":          "/var/www/html/PPNW.map",
	"ppnw":         "/var/www/html/PPNW.map",
	"ppnc":         "/var/www/html/PPNC.map",
	"ppnw_tiff":    "/var/www/html/PPNW_TIFF.map",
	"ign":          "/var/www/html/IGN.map",
	"ancien":       "/var/www/html/Ancien.map",
	"mnh":          "/var/www/html/MNH.map",
	"sambia":       "/var/www/html/Sambia.map",
	"planet":       "/var/www/html/Planet.map",
	"probos":       "/var/www/html/Probos.map",
	"mnh_wms":      "/var/www/html/MNH_WMS.map",
	"aptitude_fee": "/var/www/html/ForestimatorFEE.map",
	"aptitude_cs":  "/var/www/html/ForestimatorCS.map",
	"station_fee":  "/var/www/html/StationFEE.map",
	"forestimator": "/var/www/html/ForestimatorAutre.map",
	"ahf":          "/var/www/html/Probos_ahf.map",
	"planet_ir":    "/var/www/html/Planet_IR.map",
	"makalaya":     "/var/www/html/iDROC_Cam.map",
	"idroc/congo":  "/var/www/html/iDROC_RC.map",
	"ogf":          "/var/www/html/ogf.map",
}

type proc struct {
	proxyOld              *httputil.ReverseProxy
	downloader            *httputil.ReverseProxy
	openforis             *httputil.ReverseProxy
	started               bool
	cmd                   *exec.Cmd
	outPipe               io.ReadCloser
	errPipe               io.ReadCloser
	starttime             time.Time
	infoDirName           string
	totalRequestsReceived int

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

func startForestimatorWebServerOld(wd string) (cmd *exec.Cmd) {
	cmd = exec.Command("sh", "launchOld.sh") //, "--deploy-path=/ --docroot \"/home/carto/app/Forestimator/data/;/favicon.ico,/google52ee6b8ebe0b4b19.html,/sitemap.xml,/resources,/style,/tmp,/data,/js,/jslib,/img,/pdf,/video,resources/themes/bootstrap/5\" --http-port 8001 --http-addr 127.0.0.1 -c /home/gef/Documents/Forestimator/data/wt_config.xml --BD \"/home/gef/Documents/Forestimator/carteApt/data/aptitudeEssDB.db\" --colPath \"Dir\" ")
	cmd.Dir = wd + "Forestimator/forestimatorWeb"
	return cmd
}

func startForestimatorWebServer(wd string) (cmd *exec.Cmd) {
	cmd = exec.Command("sh", "launch.sh") //, "--deploy-path=/ --docroot \"/home/carto/app/Forestimator/data/;/favicon.ico,/google52ee6b8ebe0b4b19.html,/sitemap.xml,/resources,/style,/tmp,/data,/js,/jslib,/img,/pdf,/video,resources/themes/bootstrap/5\" --http-port 8001 --http-addr 127.0.0.1 -c /home/gef/Documents/Forestimator/data/wt_config.xml --BD \"/home/gef/Documents/Forestimator/carteApt/data/aptitudeEssDB.db\" --colPath \"Dir\" ")
	cmd.Dir = wd + "Forestimator/forestimatorWeb"
	return cmd
}

func startForestimatorDownloadServer(wd string) (cmd *exec.Cmd) {
	cmd = exec.Command("/usr/local/go/bin/go", "run", "downloadServer.go")
	cmd.Dir = wd + "Forestimator/forestimatorDownloadServer"
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
	info, err = parseProc(p, listOfProperties)
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
		openFile.Write([]byte(strings.ReplaceAll(listOfProperties, ": ", ",")))
		openFile.Write([]byte("\n"))
	}
	openFile.Write([]byte(strings.Split(time.Now().String(), "+")[0]))
	openFile.Write([]byte(","))
	for _, property := range strings.Split(listOfProperties, ": ") {
		openFile.Write([]byte(data[property]))
		openFile.Write([]byte(","))
	}
	openFile.Write([]byte("\n"))
	openFile.Close()
}

func recordRequest(p *proc, request string) {
	p.totalRequestsReceived++
	path := p.infoDirName + "/" + "requestRecord.csv"
	newfile := false
	if _, err := os.Stat(path); errors.Is(err, os.ErrNotExist) {
		newfile = true
	}
	openFile, _ := os.OpenFile(path, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
	if newfile {
		openFile.Write([]byte("date, request, meanTimeBetweenRequests(ms)"))
		openFile.Write([]byte("\n"))
	}
	openFile.Write([]byte(strings.Split(time.Now().String(), "+")[0]))
	openFile.Write([]byte(", "))
	openFile.Write([]byte(request))
	openFile.Write([]byte(", "))
	elapsed := time.Since(p.starttime)
	meanTime := float64(elapsed.Milliseconds()) / float64(p.totalRequestsReceived)
	openFile.Write([]byte(strconv.FormatFloat(meanTime, 'f', 2, 64)))
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
	text := make([]byte, bufferSize)
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
	text := make([]byte, bufferSize)
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

func normalizeKeys(v url.Values, normalFunc func(string) string) {
	for param, values := range v {
		normalizedParam := normalFunc(param)
		v.Del(param)
		// Mapserv doesn't take multiple values per param
		//  Save a little time and only set the first one
		v.Set(normalizedParam, values[0])
	}
}

func getFilenameForMapserv(request string) string {
	return mapMapservRoutes[request]
}

func handleCgiRequest(w http.ResponseWriter, r *http.Request) {

	r.ParseForm()

	normalizeKeys(r.Form, strings.ToUpper)

	if r.Form.Get("REQUEST") == "" {
		r.Form.Set("REQUEST", "GetCapabilities")
	}

	if r.Form.Get("SERVICE") == "" {
		r.Form.Set("SERVICE", "WMS")
	}

	res := strings.Split(r.URL.Path, "cgi-bin/")
	if len(res) < 2 {
		http.Error(w, "Invalid request path", http.StatusBadRequest)
		return
	}
	request := res[1]
	if request == "" {
		http.Error(w, "Invalid request path", http.StatusBadRequest)
		return
	}

	r.Form.Del("MAP")
	r.Form.Set("MAP", getFilenameForMapserv(request))

	queryString := "QUERY_STRING=" + r.Form.Encode()
	// env := append(config.Environment, queryString)
	handler := cgi.Handler{
		Path: "/usr/lib/cgi-bin/" + request,
		Env:  []string{queryString},
	}

	handler.ServeHTTP(w, r)
}

func main() {
	/*certCarto, err := tls.LoadX509KeyPair("/etc/letsencrypt/live/gxgfservcarto.gxabt.ulg.ac.be-0001/fullchain.pem", "/etc/letsencrypt/live/gxgfservcarto.gxabt.ulg.ac.be-0001/privkey.pem")
	if err != nil {
		fmt.Println("Carto certificate not found!")
	}

	certForestimator, err := tls.LoadX509KeyPair("/etc/letsencrypt/live/forestimator.gembloux.ulg.ac.be/fullchain.pem", "/etc/letsencrypt/live/forestimator.gembloux.ulg.ac.be/privkey.pem")
	if err != nil {
		fmt.Println("Forestimator certificate not found!")
	}

	getCertificates := func(hello *tls.ClientHelloInfo) (*tls.Certificate, error) {
		switch hello.ServerName {
		case "gxgfservcarto.gxabt.ulg.ac.be":
			return &certCarto, nil
		case "forestimator.gembloux.ulg.ac.be":
			return &certForestimator, nil
		default:
			return nil, fmt.Errorf("no certificate for %s", hello.ServerName)
		}
	}*/

	/*tlsConfig := &tls.Config{
		GetCertificate: getCertificates,
	}
	*/
	forestimator := proc{
		started:               false,
		cmd:                   nil,
		outPipe:               nil,
		pid:                   -1,
		totalRequestsReceived: 0,
	}
	forestimatorOld := proc{
		started:               false,
		cmd:                   nil,
		outPipe:               nil,
		pid:                   -1,
		totalRequestsReceived: 0,
	}

	go func() {
		for {
			forestimatorOld.cmd = startForestimatorWebServerOld(workingDirectory)
			log.Println(forestimatorOld.cmd.Args)
			forestimatorOld.outPipe, _ = forestimatorOld.cmd.StdoutPipe()
			forestimatorOld.errPipe, _ = forestimatorOld.cmd.StderrPipe()
			err := forestimatorOld.cmd.Start()
			if err == nil {
				forestimatorOld.starttime = time.Now()
				log.Println(forestimatorOld.cmd.Args)
				go func() {
					time.Sleep(100 * time.Millisecond)
					forestimatorOld.pid = getForestimatorPid("forestimatorWeb")
					t := strconv.Itoa(time.Now().Year()) + "." +
						time.Now().Month().String() + "." +
						strconv.Itoa(time.Now().Day()) + "." +
						strconv.Itoa(time.Now().Hour()) + "." + strconv.Itoa(time.Now().Minute()) + "." + strconv.Itoa(time.Now().Second()) + "."
					forestimatorOld.infoDirName = workingDirectory + "/Forestimator/forestimatorMiddleware/logs/" + t + strconv.Itoa(forestimatorOld.pid)
					os.Mkdir(forestimatorOld.infoDirName, os.ModePerm)
					forestimatorOld.started = true
					log.Println("Started Forestimator web server. pid: " + strconv.Itoa(forestimatorOld.pid))
				}()
			} else {
				log.Println("Failed to start Forestimator web server")
				log.Println(err)
			}
			forestimatorOld.cmd.Wait()
			forestimatorOld.started = false
			recordEndLog(&forestimatorOld)
			log.Println("Forestimator web server has stopped")
			log.Println("Restarting now...")
		}
	}()
	go func() {
		for {
			cmd := startForestimatorDownloadServer(workingDirectory)
			cmd.Start()
			cmd.Wait()
			log.Println("Forestimator download server has stopped")
			log.Println("Restarting now...")
		}

	}()
	go func() {
		forestimatorOld.proxyOld, _ = NewProxy("http://localhost:" + portForestimatorWebOld)
		forestimatorOld.downloader, _ = NewProxy("http://localhost:" + portForestimatorDownloader)
		forestimatorOld.openforis, _ = NewProxy("http://localhost:" + portOpenForis)
		for {
			if forestimatorOld.started {
				director := forestimatorOld.proxyOld.Director
				forestimatorOld.proxyOld.Director = func(r *http.Request) {
					director(r)
					recordRequest(&forestimatorOld, r.RemoteAddr+r.RequestURI)
					/*if strings.Contains(r.RequestURI, "?signal=") && strings.Contains(r.RequestURI, "&wtd=") {
						log.Println("Signal detected in request URI")
						r.URL.RawQuery = "GET /"
						return
					}*/
				}

				/*mapservHandler := http.NewServeMux()
				mapservHandler.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
					handleDownload(w, r, "/var/www/html/")
				})
									mapservHandler.HandleFunc("/cgi-bin/", handleCgiRequest)
				f*/

				forestimatorHandler := http.NewServeMux()
				forestimatorHandler.Handle("/collect/", forestimatorOld.openforis)
				forestimatorHandler.Handle("/", forestimatorOld.proxyOld)
				forestimatorHandler.HandleFunc("/robots.txt", sendRobotsTxt)
				forestimatorHandler.HandleFunc("/llms.txt", sendLLMsTxt)
				forestimatorHandler.Handle("/results/", forestimatorOld.downloader)
				forestimatorHandler.HandleFunc("/cgi-bin/", handleCgiRequest)

				fmt.Println(http.ListenAndServe(":"+portMiddleWare, forestimatorHandler))

				/*listener, err := tls.Listen("tcp", portMiddleware, tlsConfig)
				?if err != nil {
					log.Fatalf("Failed to listen on port 443: %v", err)
				}*/

				/*mapserveur := &http.Server{
					Addr: ":443",
					Handler: http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
						mux1.ServeHTTP(w, r)
					}),
					TLSConfig: &tls.Config{Certificates: []tls.Certificate{certCarto}},
				}
				carto := &http.Server{
					Addr: ":443",
					Handler: http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
						mux2.ServeHTTP(w, r)
					}),
					TLSConfig: &tls.Config{Certificates: []tls.Certificate{certForestimator}},
				}*/

				/*http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
					switch r.Host {
					case "gxgfservcarto.gxabt.ulg.ac.be":
						mux1.ServeHTTP(w, r)
					case "forestimator.gembloux.ulg.ac.be":
						mux2.ServeHTTP(w, r)
					}
				})
				log.Println(http.Serve(listener, nil))*/

				log.Println("Proxy server has stopped")
				log.Println("Restarting now...")
			}
		}
	}()
	go func() {
		timeup := true
		for {
			if forestimatorOld.started && !timeup {
				record, _ := refreshProcInfo(&forestimatorOld)
				recordData(&forestimatorOld, record)
				timeup = true
			} else {
				time.Sleep(updateIntervall)
				timeup = false
			}
		}
	}()
	go func() {
		timeup := true
		tmp := []string{}
		for {
			if forestimatorOld.started && !timeup {
				processInfo := getProcessInfo(&forestimatorOld)
				if processInfo != "" {
					tmp = append(tmp, strings.TrimSpace(processInfo))
				}
				timeup = true
			} else {
				if len(tmp) > 10 {
					for _, str := range tmp {
						if str != "" {
							recordThisInFile(false, strings.Split(time.Now().String(), "+")[0]+","+str+","+strconv.Itoa(forestimatorOld.vmPeak), "threadSSig.csv", forestimatorOld.infoDirName)
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
			if forestimatorOld.started {
				timeup = false
			}
		}
	}()
	go func() {
		timeup := true
		for {
			if forestimatorOld.started && !timeup {
				recordTerminalLogs(&forestimatorOld)
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
			if forestimatorOld.started && !timeup {
				recordErrorLogs(&forestimatorOld)
				timeup = true
			} else {
				time.Sleep(time.Millisecond * 10)
				timeup = false
			}
		}
	}()
	select {}
}
