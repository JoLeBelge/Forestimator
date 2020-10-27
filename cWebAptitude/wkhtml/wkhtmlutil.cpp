#include "wkhtmlutil.h"

int wkhtml(std::string html) {
    wkhtmltoimage_global_settings * gs;
    wkhtmltoimage_converter * c;
    const unsigned char * data;
    long len;

    std::ofstream ofs ("/home/lisein/Documents/carteApt/Forestimator/data/tmp.html", std::ofstream::out);
    ofs << html;
    ofs.close();

    /* Init wkhtmltoimage in graphics less mode */
    wkhtmltoimage_init(false);

    /*
     * Create a global settings object used to store options that are not
     * related to input objects, note that control of this object is parsed to
     * the converter later, which is then responsible for freeing it
     */
    gs = wkhtmltoimage_create_global_settings();

    /* We want to convert the qstring documentation page */
    // input ; soit une url, soit un fichier html en local. on ne peux pas donner directement le text hml en input
    wkhtmltoimage_set_global_setting(gs, "in", "/home/lisein/Documents/carteApt/Forestimator/data/tmp.html");

    // si out est spécifié dans les options, len et data seront vide.
    wkhtmltoimage_set_global_setting(gs, "out", "/home/lisein/Documents/carteApt/Forestimator/data/tmp/img.jpeg");
    wkhtmltoimage_set_global_setting(gs, "fmt", "jpeg");

    /* Create the actual converter object used to convert the pages */
    c = wkhtmltoimage_create_converter(gs, NULL);

    wkhtmltoimage_set_error_callback(c, error);

    /* Call the progress_changed function when progress changes
    wkhtmltoimage_set_progress_changed_callback(c, progress_changed);


    wkhtmltoimage_set_phase_changed_callback(c, phase_changed);




    wkhtmltoimage_set_warning_callback(c, warning);

     Perform the actual conversion */
    if (!wkhtmltoimage_convert(c))
        fprintf(stderr, "Conversion failed!");

    /* Output possible http error code encountered */
    printf("httpErrorCode: %d\n", wkhtmltoimage_http_error_code(c));

    len = wkhtmltoimage_get_output(c, &data);
    printf("%ld len\n", len);

    /* Destroy the converter object since we are done with it */
    wkhtmltoimage_destroy_converter(c);

    /* We will no longer be needing wkhtmltoimage funcionality */
    wkhtmltoimage_deinit();

    return 0;
}


/* Print a message to stderr when an error occurs */
void error(wkhtmltoimage_converter * c, const char * msg) {
    fprintf(stderr, "Error: %s\n", msg);
}
