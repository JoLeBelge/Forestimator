package forestimatormiddleware

import (
	"fmt"
	"log"
	"net/http"
	"net/http/httputil"
	"net/url"
)

func main() {
	// Define target servers for reverse proxies
	webSiteTarget, err := url.Parse("http://localhost:47000")
	if err != nil {
		log.Fatal(err)
	}

	devTarget, err := url.Parse("http://localhost:47100")
	if err != nil {
		log.Fatal(err)
	}

	// Create reverse proxies
	webSiteProxy := httputil.NewSingleHostReverseProxy(webSiteTarget)
	devProxy := httputil.NewSingleHostReverseProxy(devTarget)

	// Set up handlers with path prefix stripping
	http.HandleFunc("/webSite/", func(w http.ResponseWriter, r *http.Request) {
		r.Host = webSiteTarget.Host
		webSiteProxy.ServeHTTP(w, r)
	})
	http.HandleFunc("/dev/", func(w http.ResponseWriter, r *http.Request) {
		r.Host = devTarget.Host
		devProxy.ServeHTTP(w, r)
	})

	// Start server on port 8085
	fmt.Println("Server starting on :8085")
	log.Fatal(http.ListenAndServe(":8085", nil))
}
