#include "rasterclipresource.h"
#include <boost/filesystem.hpp>
#include <cmath>
#include <vector>

//extern int maxSizePix4Export;

void rasterClipResource::handleRequest(const Http::Request &request, Http::Response &response)
{
    std::cout << "rasterClipResource:: handle request" << std::endl;
    char **papszOptions = nullptr;
    auto params = request.urlParams();
    std::string lCode;
    double xmin = 0.0;
    double ymin = 0.0;
    double xmax = 0.0;
    double ymax = 0.0;
    int coordCount = 0;

    for (const auto &param : params)
    {
        const auto &name = param.first;
        const auto &value = param.second;

        if (name == "layerCode")
        {
            lCode = value;
        }
        else if (name == "xmin")
        {
            xmin = std::stod(value);
            ++coordCount;
        }
        else if (name == "xmax")
        {
            xmax = std::stod(value);
            ++coordCount;
        }
        else if (name == "ymin")
        {
            ymin = std::stod(value);
            ++coordCount;
        }
        else if (name == "ymax")
        {
            ymax = std::stod(value);
            ++coordCount;
        }
    }

    std::shared_ptr<layerBase> l = mDico->getLayerBase(lCode);

    GDALAllRegister();

    if (!l || !l->rasterExist())
    {
        response.addHeader("Content-Type", "text/plain; charset=utf-8");
        response.out() << " attention, un des fichiers input n'existe pas : " << (l ? l->getPathTif() : std::string("null")) << std::endl;
        if (papszOptions != nullptr)
        {
            CSLDestroy(papszOptions);
        }
        return;
    }

    std::string pathTif = l->getPathTif();
    const std::string inputPath = pathTif;
    GDALDataset *pInputRaster = nullptr;
    GDALDataset *pCroppedRaster = nullptr;
    GDALDriver *pDriver = nullptr;

    constexpr const char *pszFormat = "GTiff";
    pDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
    pInputRaster = static_cast<GDALDataset *>(GDALOpen(inputPath.c_str(), GA_ReadOnly));

    if (pInputRaster == nullptr)
    {
        response.addHeader("Content-Type", "text/plain; charset=utf-8");
        response.out() << " erreur : impossible d'ouvrir le fichier raster : " << inputPath << std::endl;
        if (papszOptions != nullptr)
        {
            CSLDestroy(papszOptions);
        }
        return;
    }

    boost::filesystem::path tmpPath = boost::filesystem::path(mDico->File("TMPDIR")) / boost::filesystem::unique_path("tmp-%%%%-%%%%-%%%%.tif");
    std::string output = tmpPath.string();

    double transform[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    double tr1[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    pInputRaster->GetGeoTransform(transform);

    papszOptions = CSLSetNameValue(papszOptions, "COMPRESS", "DEFLATE");

    if (coordCount == 4)
    {
        OGREnvelope ext;
        ext.MaxY = ymax;
        ext.MinY = ymin;
        ext.MaxX = xmax;
        ext.MinX = xmin;

        std::cout << " cropIm layer " << l->getPathTif() << " on extend " << xmin << ", " << ymin << ", " << xmax << ", " << ymax << std::endl;

        pInputRaster->GetGeoTransform(tr1);

        OGREnvelope extGlob;
        extGlob.MaxY = transform[3];
        extGlob.MinX = transform[0];
        extGlob.MinY = transform[3] + transform[5] * pInputRaster->GetRasterBand(1)->GetYSize();
        extGlob.MaxX = transform[0] + transform[1] * pInputRaster->GetRasterBand(1)->GetXSize();

        // intersect the two extents
        ext.Intersect(extGlob);

        double width = (ext.MaxX - ext.MinX);
        double height = (ext.MaxY - ext.MinY);

        // adjust top left coordinates
        transform[0] = ext.MinX;
        transform[3] = ext.MaxY;

        // determine dimensions of the new (cropped) raster in cells
        const int xSize = static_cast<int>(std::round(width / transform[1]));
        const int ySize = static_cast<int>(std::round(height / transform[1]));

        pCroppedRaster = pDriver->Create(output.c_str(), xSize, ySize, 1, pInputRaster->GetRasterBand(1)->GetRasterDataType(), papszOptions);
        if (pCroppedRaster == nullptr)
        {
            GDALClose(pInputRaster);
            response.addHeader("Content-Type", "text/plain; charset=utf-8");
            response.out() << " erreur : impossible de créer le raster croppé" << std::endl;
            if (papszOptions != nullptr)
            {
                CSLDestroy(papszOptions);
            }
            return;
        }

        pCroppedRaster->SetProjection(pInputRaster->GetProjectionRef());
        pCroppedRaster->SetGeoTransform(transform);

        const int xOffset = static_cast<int>(std::round((transform[0] - tr1[0]) / tr1[1]));
        const int yOffset = static_cast<int>(std::round((transform[3] - tr1[3]) / tr1[5]));

        std::vector<float> scanline(static_cast<size_t>(xSize));
        for (int row = 0; row < ySize; ++row)
        {
            CPLErr errRead = pInputRaster->GetRasterBand(1)->RasterIO(GF_Read, xOffset, row + yOffset, xSize, 1, scanline.data(), xSize, 1, GDT_Float32, 0, 0);
            if (errRead != CE_None)
            {
                // close datasets and return error
                GDALClose(pInputRaster);
                GDALClose(pCroppedRaster);
                response.addHeader("Content-Type", "text/plain; charset=utf-8");
                response.out() << " erreur lors de la lecture du raster source" << std::endl;
                if (papszOptions != nullptr)
                {
                    CSLDestroy(papszOptions);
                }
                return;
            }

            CPLErr errWrite = pCroppedRaster->GetRasterBand(1)->RasterIO(GF_Write, 0, row, xSize, 1, scanline.data(), xSize, 1, GDT_Float32, 0, 0);
            if (errWrite != CE_None)
            {
                GDALClose(pInputRaster);
                GDALClose(pCroppedRaster);
                response.addHeader("Content-Type", "text/plain; charset=utf-8");
                response.out() << " erreur lors de l'écriture du raster croppé" << std::endl;
                if (papszOptions != nullptr)
                {
                    CSLDestroy(papszOptions);
                }
                return;
            }
        }

        // define color palette for the cropped raster
        l->createRasterColorInterpPalette(pCroppedRaster->GetRasterBand(1));

        // close datasets
        GDALClose(pInputRaster);
        GDALClose(pCroppedRaster);

        std::ifstream r(output.c_str(), std::ios::in | std::ios::binary);
        response.addHeader("Content-Type", "image/tiff");
        response.out() << r.rdbuf();
        r.close();
    }
    else
    {
        // refuse export for too fine resolution rasters
        if (transform[1] >= 10.0)
        {
            pCroppedRaster = pDriver->CreateCopy(output.c_str(), pInputRaster, FALSE, papszOptions, nullptr, nullptr);
            if (pCroppedRaster != nullptr)
            {
                GDALClose(pCroppedRaster);
            }

            pCroppedRaster = static_cast<GDALDataset *>(GDALOpen(output.c_str(), GA_Update));
            if (pCroppedRaster != nullptr)
            {
                l->createRasterColorInterpPalette(pCroppedRaster->GetRasterBand(1));
                GDALClose(pCroppedRaster);
            }

            GDALClose(pInputRaster);

            std::ifstream r(output.c_str(), std::ios::in | std::ios::binary);
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

    if (papszOptions != nullptr)
    {
        CSLDestroy(papszOptions);
    }
}
