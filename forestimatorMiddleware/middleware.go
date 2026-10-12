package main

import (
	"log"
	"net/http"
	"net/http/httputil"
	"net/url"
	"strings"
	"time"
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
	appProxy := newProxy("http://localhost:47000")
	apiProxy := newProxy("http://localhost:47100")

	mux := http.NewServeMux()

	mux.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		switch strings.ToLower(r.Host) {
		case "forestimator.gembloux.ulg.ac.be":
			appProxy.ServeHTTP(w, r)

		case "forestimator.gembloux.ulg.ac.be/dev":
			apiProxy.ServeHTTP(w, r)

		default:
			http.Error(w, "Unknown host", http.StatusNotFound)
		}
	})

	server := &http.Server{
		Addr:              ":8085",
		Handler:           mux,
		ReadHeaderTimeout: 5 * time.Second,
		ReadTimeout:       30 * time.Second,
		WriteTimeout:      0,
		IdleTimeout:       120 * time.Second,
	}

	log.Printf("reverse proxy listening on %s", server.Addr)
	log.Fatal(server.ListenAndServe())
}
