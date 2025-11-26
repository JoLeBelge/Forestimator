#include "rasterclipresource.h"
#include <boost/filesystem.hpp>

extern int maxSizePix4Export;

void rasterClipResource::handleRequest(const Http::Request &request, Http::Response &response)
{
    std::cout << "rasterClipResource:: handle request" << std::endl;
    char **papszOptions = NULL;
    auto params = request.urlParams();
    std::string lCode("");
    double xmin, ymin, xmax, ymax;
    int test = 0;
    for (const auto &param : params)
    {
        const auto &name = param.first;
        const auto &value = param.second;

        if (name == "layerCode")
        {
            lCode = value;
        }
        if (name == "xmin")
        {
            xmin = std::stod(value);
            test++;
        }
        if (name == "xmax")
        {
            xmax = std::stod(value);
            test++;
        }
        if (name == "ymin")
        {
            ymin = std::stod(value);
            test++;
        }
        if (name == "ymax")
        {
            ymax = std::stod(value);
            test++;
        }
    }
    std::shared_ptr<layerBase> l = mDico->getLayerBase(lCode);

    GDALAllRegister();
    if (l->rasterExist())
    {

        std::string pathTif = l->getPathTif();
        std::string inputPath = pathTif;
        GDALDataset *pInputRaster, *pCroppedRaster;

        GDALDriver *pDriver;

        const char *pszFormat = "GTiff";
        pDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
        pInputRaster = (GDALDataset *)GDALOpen(inputPath.c_str(), GA_ReadOnly);

        // Check if GDALOpen failed
        if (pInputRaster == NULL)
        {
            response.addHeader("Content-Type", "text/plain; charset=utf-8");
            response.out() << " erreur : impossible d'ouvrir le fichier raster : " << inputPath << std::endl;
            return;
        }

        // Use boost::filesystem::unique_path to create a safe unique temporary filename
        boost::filesystem::path tmpPath = boost::filesystem::path(mDico->File("TMPDIR")) / boost::filesystem::unique_path("tmp-%%%%-%%%%-%%%%.tif");
        std::string output = tmpPath.string();

        double transform[6], tr1[6];
        pInputRaster->GetGeoTransform(transform);

        papszOptions = CSLSetNameValue(papszOptions, "COMPRESS", "DEFLATE");

        if (test == 4)
        {

            OGREnvelope ext = OGREnvelope();
            ext.MaxY = ymax;
            ext.MinY = ymin;
            ext.MaxX = xmax;
            ext.MinX = xmin;

            std::cout << " cropIm layer " << l->getPathTif() << " on extend " << xmin << ", " << ymin << ", " << xmax << ", " << ymax << std::endl;

            pInputRaster->GetGeoTransform(tr1);

            OGREnvelope extGlob = OGREnvelope();
            extGlob.MaxY = transform[3];
            extGlob.MinX = transform[0];
            extGlob.MinY = transform[3] + transform[5] * pInputRaster->GetRasterBand(1)->GetYSize();
            extGlob.MaxX = transform[0] + transform[1] * pInputRaster->GetRasterBand(1)->GetXSize();
            // garder l'intersect des 2 extend
            ext.Intersect(extGlob);

            double width((ext.MaxX - ext.MinX)), height((ext.MaxY - ext.MinY));

            // adjust top left coordinates
            transform[0] = ext.MinX;
            transform[3] = ext.MaxY;
            // determine dimensions of the new (cropped) raster in cells
            int xSize = round(width / transform[1]);
            int ySize = round(height / transform[1]);

            // create the new (cropped) dataset

            pCroppedRaster = pDriver->Create(output.c_str(), xSize, ySize, 1, pInputRaster->GetRasterBand(1)->GetRasterDataType(), papszOptions);
            pCroppedRaster->SetProjection(pInputRaster->GetProjectionRef());
            pCroppedRaster->SetGeoTransform(transform);

            int xOffset = round((transform[0] - tr1[0]) / tr1[1]);
            int yOffset = round((transform[3] - tr1[3]) / tr1[5]);
            float *scanline;
            scanline = (float *)CPLMalloc(sizeof(float) * xSize);
            // boucle sur chaque ligne
            for (int row = 0; row < ySize; row++)
            {
                // lecture
                pInputRaster->GetRasterBand(1)->RasterIO(GF_Read, xOffset, row + yOffset, xSize, 1, scanline, xSize, 1, GDT_Float32, 0, 0);
                // écriture
                pCroppedRaster->GetRasterBand(1)->RasterIO(GF_Write, 0, row, xSize, 1, scanline, xSize, 1, GDT_Float32, 0, 0);
            }
            CPLFree(scanline);
            // on définit la palette de couleur associé à ce raster croppé
            l->createRasterColorInterpPalette(pCroppedRaster->GetRasterBand(1));
            GDALClose(pInputRaster);
            if (pCroppedRaster != NULL)
            {
                GDALClose((GDALDatasetH)pCroppedRaster);
            }
            std::ifstream r(output, std::ios::in | std::ios::binary);
            response.addHeader("Content-Type", "image/tiff");
            response.out() << r.rdbuf();
            r.close();
        }
        else
        {
            // je met des test pour sécuriser le fait qu'on ne demande pas le téléchargement de couche raster trop lourde; test sur la résolution?
            if (transform[1] >= 10)
            {
                pCroppedRaster = pDriver->CreateCopy(output.c_str(), pInputRaster, FALSE, papszOptions, NULL, NULL);
                if (pCroppedRaster != NULL)
                {
                    GDALClose((GDALDatasetH)pCroppedRaster);
                }
                // je le referme et réouvre car sinon "Cannot modify tag "PhotometricInterpretation" while writing"
                pCroppedRaster = (GDALDataset *)GDALOpen(output.c_str(), GA_Update);

                // on définit la palette de couleur associé à ce raster croppé
                l->createRasterColorInterpPalette(pCroppedRaster->GetRasterBand(1));
                GDALClose(pInputRaster);
                if (pCroppedRaster != NULL)
                {
                    GDALClose((GDALDatasetH)pCroppedRaster);
                }
                std::ifstream r(output, std::ios::in | std::ios::binary);
                response.addHeader("Content-Type", "image/tiff");
                response.out() << r.rdbuf();
                r.close();
            }
            else
            {
                GDALClose(pInputRaster);
                response.addHeader("Content-Type", "text/plain; charset=utf-8");
                response.out() << " la résolution de ce raster (" << l->Code() << ") est trop fine, vous ne pouvez pas utiliser cet outil pour son téléchargement. Résolution : " << transform[1] << " m par pixel" << std::endl;
            }
        }
    }
    else
    {

        response.addHeader("Content-Type", "text/plain; charset=utf-8");
        response.out() << " attention, un des fichiers input n'existe pas : " << l->getPathTif() << std::endl;
    }

    // Cleanup CSL options
    if (papszOptions != NULL)
    {
        CSLDestroy(papszOptions);
    }
}
