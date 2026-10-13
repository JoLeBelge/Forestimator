package main

import (
	"crypto/tls"
	"fmt"
	"log"
	"net/http"
	"net/http/httputil"
	"net/url"
	"strings"
)

func newProxy(target string) *httputil.ReverseProxy {
	u, err := url.Parse(target)
	if err != nil {
		log.Fatalf("invalid backend URL %q: %v", target, err)
	}

	proxy := httputil.NewSingleHostReverseProxy(u)

	originalDirector := proxy.Director
	proxy.Director = func(req *http.Request) {
		originalDirector(req)

		// Set forwarding information.
		clientIP := req.RemoteAddr
		if host, _, err := strings.Cut(req.RemoteAddr, ":"); err {
			clientIP = host
		}
		req.Header.Set("X-Forwarded-For", clientIP)
		req.Header.Set("X-Forwarded-Proto", "https")
		req.Header.Set("X-Forwarded-Host", req.Host)
	}

	proxy.ErrorHandler = func(
		w http.ResponseWriter,
		r *http.Request,
		err error,
	) {
		log.Printf("proxy error: %v", err)
		http.Error(w, "Bad Gateway", http.StatusBadGateway)
	}

	return proxy
}

func main() {
	/*certCarto, err := tls.LoadX509KeyPair("/etc/letsencrypt/live/gxgfservcarto.gxabt.ulg.ac.be-0001/fullchain.pem", "/etc/letsencrypt/live/gxgfservcarto.gxabt.ulg.ac.be-0001/privkey.pem")
	if err != nil {
		fmt.Println("Carto certificate not found!")
	}*/

	certForestimator, err := tls.LoadX509KeyPair("/etc/letsencrypt/live/forestimator.gembloux.ulg.ac.be/fullchain.pem", "/etc/letsencrypt/live/forestimator.gembloux.ulg.ac.be/privkey.pem")
	if err != nil {
		fmt.Println("Forestimator certificate not found!")
	}

	certAppli, err := tls.LoadX509KeyPair("/etc/letsencrypt/live/appliprfw.gembloux.ulg.ac.be/fullchain.pem", "/etc/letsencrypt/live/appliprfw.gembloux.ulg.ac.be/privkey.pem")
	if err != nil {
		fmt.Println("Appli certificate not found!")
	}

	getCertificates := func(hello *tls.ClientHelloInfo) (*tls.Certificate, error) {
		switch hello.ServerName {
		case "gxgfservcarto.gxabt.ulg.ac.be":
			return nil, nil
		case "forestimator.gembloux.ulg.ac.be":
			return &certForestimator, nil
		case "appliprfw.gembloux.ulg.ac.be":
			return &certAppli, nil
		default:
			return nil, fmt.Errorf("no certificate for %s", hello.ServerName)
		}
	}

	tlsConfig := &tls.Config{
		GetCertificate: getCertificates,
	}

	siteProxy := newProxy("http://localhost:47000")
	devProxy := newProxy("http://localhost:47100")

	mux := http.NewServeMux()

	mux.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		switch strings.ToLower(r.Host) {
		case "forestimator.gembloux.ulg.ac.be":
			if r.URL.Path == "/dev" {
				r.URL.Path = "/"
				devProxy.ServeHTTP(w, r)
			}
			siteProxy.ServeHTTP(w, r)

		case "gxgfservcarto.gxabt.ulg.ac.be":
			devProxy.ServeHTTP(w, r)

		default:
			http.Error(w, "Unknown host", http.StatusNotFound)
		}
	})

	/*server := &http.Server{
		Addr:              ":8085",
		Handler:           mux,
		ReadHeaderTimeout: 5 * time.Second,
		ReadTimeout:       30 * time.Second,
		WriteTimeout:      0,
		IdleTimeout:       120 * time.Second,
	}*/

	listener, err := tls.Listen("tcp", ":443", tlsConfig)
	if err != nil {
		log.Fatalf("Failed to listen on port 443: %v", err)
	}

	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		switch r.Host {
		case "forestimator.gembloux.ulg.ac.be":
			if r.URL.Path == "/dev" {
				r.URL.Path = "/"
				devProxy.ServeHTTP(w, r)
			}
			siteProxy.ServeHTTP(w, r)

		case "gxgfservcarto.gxabt.ulg.ac.be":
			devProxy.ServeHTTP(w, r)

		default:
			http.Error(w, "Unknown host", http.StatusNotFound)
		}
	})
	//listener.Close()
	log.Println(http.Serve(listener, nil))

	//log.Printf("reverse proxy listening on %s", server.Addr)
	//log.Fatal(server.ListenAndServe())

}
