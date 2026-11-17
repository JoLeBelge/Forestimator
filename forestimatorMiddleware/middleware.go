package main

import (
	"crypto/tls"
	"encoding/json"
	"fmt"
	"log"
	"net"
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

func addCorsHeaders(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		w.Header().Set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS")
		next.ServeHTTP(w, r)
	})
}

type GeoResponse struct {
	Status  string `json:"status"`
	Country string `json:"countryCode"`
	Query   string `json:"query"`
}

func getCountryFromIP(ip string) (string, error) {
        if ip[0] == '1' && ip[1] == '0' {
               return "intern", nil
        }
	url := fmt.Sprintf("http://ip-api.com/json/%s?fields=status,countryCode", ip)
	resp, err := http.Get(url)
	if err != nil {
		return "", err
	}
	defer resp.Body.Close()
	var geo GeoResponse
	if err := json.NewDecoder(resp.Body).Decode(&geo); err != nil {
		return "", err
	}
	if geo.Status != "success" {
		return "", fmt.Errorf("GeoIP lookup failed for %s", ip)
	}
	return geo.Country, nil
}

func main() {
	allowedCountries := []string{"intern","BE", "FR", "DE", "LU", "NL", "CH"}
	certCarto, err := tls.LoadX509KeyPair("/etc/letsencrypt/live/gxgfservcarto.gxabt.ulg.ac.be-0001/fullchain.pem", "/etc/letsencrypt/live/gxgfservcarto.gxabt.ulg.ac.be-0001/privkey.pem")
	if err != nil {
		fmt.Println("Carto certificate not found!")
	}

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
			return &certCarto, nil
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

	listener, err := tls.Listen("tcp", ":443", tlsConfig)
	if err != nil {
		log.Fatalf("Failed to listen on port 443: %v", err)
	}

	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		ip := r.Header.Get("X-Forwarded-For")
		if ip == "" {
			ip, _, _ = net.SplitHostPort(r.RemoteAddr)
		} else {
			ips := strings.Split(ip, ",")
			ip = strings.TrimSpace(ips[0])
		}
		country, err := getCountryFromIP(ip)
		if err != nil {
			fmt.Printf("[ERROR] IP %s: %v", ip, err)
			http.Error(w, "Geolocation lookup failed", http.StatusInternalServerError)
			return
		}
		isAllowed := false
		for _, allowed := range allowedCountries {
			if strings.EqualFold(country, allowed) {
				isAllowed = true
				break
			}
		}
		if !isAllowed {
			log.Printf("[DENIED] IP: %s | Country: %s", ip, country)
			http.Error(w, "Access denied from your location", http.StatusForbidden)
			return
		}
		log.Printf("[ALLOWED] IP: %s | Country: %s", ip, country)

		switch r.Host {
		case "forestimator.gembloux.ulg.ac.be":
			if strings.Contains(r.URL.Path, "/dev") {
				r.URL.Path = strings.TrimPrefix(r.URL.Path, "/dev")
				devProxy.ServeHTTP(w, r)
			}
			siteProxy.ServeHTTP(w, r)

		case "gxgfservcarto.gxabt.ulg.ac.be":
			addCorsHeaders(siteProxy).ServeHTTP(w, r)

		default:
			http.Error(w, "Unknown host", http.StatusNotFound)
		}
	})
	log.Println(http.Serve(listener, nil))
}
