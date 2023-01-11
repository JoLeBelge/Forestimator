2023 01 04
Simon Tossens me signale quelques zones qui sont foireuses sur la carte d'apport en eau de 2019.
Je met les scripts R dans le dépot forestimator pour archivage. mais ces scripts ne respectent plus ma manière de travailler:
-il vaux mieux utiliser la CNSW rasterisée (voir script R pour catalogue station de Simon T)
-les packages R utilisés sont dépassé, remplacer par terra et sp

je fait ça ci-dessous, et je met à jour la carte d'aléa d'inondation (version 2020)

# 2023 01 - JL - je repart de mon script de 2016 mais je vire la CNSW au format shp tuilée pour remplacer par CNSW au format raster. j'en profite pour utiliser terra et sp à la place de raster et maptool/rgdal

require(RSQLite)
require(terra)
require(sf)

setwd("/home/jo/Documents/Carto/NH_RW2019")

# effectue un aggregate ou disaggregate pour arriver le plus proche possible d'une r?solution renseign?e par l'utilisateur
DeZoom <- function(rast,
                   res = 10,
                   method = '',
                   my.fun = mean) {
  fact <- res / xres(rast)
  if (round(fact) > 1) {
    raster.resampled <-
      aggregate(rast, fact = round(fact), fun = my.fun)
  } else{
    raster.resampled <- disagg(rast, fact = round(1 / fact), method = method)
  }
  return(raster.resampled)
}

# Appel des dicos dans db
db.path <-
  "/home/jo/app/Forestimator/carteApt/data/aptitudeEssDB.db"
db <- dbConnect(SQLite(), dbname = db.path)
dbBegin(db)
dico_cnsw <- dbReadTable(db , "zz_Table_sigles_eclates")
dbDisconnect(db) # fermer la BD apres utilisation
rm(db)

path.cnsw <- "/home/jo/Documents/Carto/pH/2020/CNSW.tif"
MNT.path = "/home/jo/Documents/Carto/MNT/MNT_10m_WALLONIA.tif"
# version 2020 de la carte Alea inondation. Je commence par merger les shp qui sont découpé par province
#ogrmerge.py -single -f GPKG -o aleaInondation2020.gpkg  ALEA_SHAPE_31370_PROV_BRABANT_WALLON/ALEA_INOND__ALEA.shp ALEA_SHAPE_31370_PROV_LIEGE/ALEA_INOND__ALEA.shp ALEA_SHAPE_31370_PROV_NAMUR/ALEA_INOND__ALEA.shp ALEA_SHAPE_31370_PROV_HAINAUT/ALEA_INOND__ALEA.shp ALEA_SHAPE_31370_PROV_LUXEMBOURG/ALEA_INOND__ALEA.shp
# rstat bug si j'ouvre le shp de 2 g avec read_sf, donc j'effectue le raterize avec gdal
# je calcule les surface pour virer les polygones de moins de 150 m2

#gdal_rasterize -te 42250.0 21170.0 295170.0 167700.0 -tr 10 10 -ot UInt16 -a CODEALEA -l selected aleaInondation2020.gpkg aleaInondation2020.tif

path.hydro = "/home/jo/Documents/Carto/NH_RW2019/data/hydro/reseau_hydro_scenic_tmp.shp"

#r.cnsw <- rast(path.cnsw) # carte apport nappe cnsw fait une seule fois
r.mnt <- rast(MNT.path)
v.hydro <-
  read_sf(dsn = "/home/jo/Documents/Carto/NH_RW2019/data/hydro/", layer = "reseau_hydro_scenic_tmp")


rast.ai.path <- "/home/jo/Documents/Carto/NH_RW2019/data/alea_Inondation2022/aleaInondation2020.tif"

#v.ai <-
  #read_sf(dsn = "/home/jo/Documents/Carto/NH_RW2019/data/alea_inondation_20070626", layer = "alea_inondation_20070626")
  #read_sf(dsn = "/home/jo/Documents/Carto/NH_RW2019/data/alea_Inondation2022/aleaInondation2020.gpkg", layer = "merged")
# al?a d'inondation
#r.ai <- rasterize(v.ai, r.ae, 1)

r.ai <- rast(rast.ai.path)

resolution = 10
# vallon encaiss?, bas de versant : 75 m, comme Fran?ois Rid, mais 2 tpi, 1 basic et 1 en pond?ration inv distance.
rayon.tpi.norm = 75
rayon.pente = 20
#pour large vall?e plate
rayon.tpi.inv = 200
# petit vallon encaiss?
#,rayon.tpi.norm=100
seuil.tpi = -12
META = T
filename <- "AE_W_202301"

r.ae <- init(r.ai, 0)

# buffer autour du reseau hydro : utile uniquement pour apport variable ?coulement temporaire

temporaire <- v.hydro[v.hydro$PERMANENCE != 1, ]
buf.hydro.tempo <- st_buffer(temporaire, 30)
# retier les géométrie vide sinon bug dans rasterize
v <- buf.hydro.tempo[!st_is_empty(buf.hydro.tempo), ]
r.buf.hydro.tempo <- rasterize(v, r.ae, 1)
buf.hydro.perm <- st_buffer(v.hydro[v.hydro$PERMANENCE == 1, ], 30)
r.buf.hydro.perm <-
  rasterize(buf.hydro.perm[!st_is_empty(buf.hydro.perm), ], r.ae, 1)

hydro.r <- rasterize(v.hydro, r.ae, 1)

#writeRaster(hydro.r, "test_hydro.tif", overwrite=TRUE

cat("D?tection des apport permanent en provenance d'une nappe phr?atique (sur base p?do) \n")

apport.nappe.path <- "apportNappe.tif"
# un peu long alors je sauve le résultat en dur pour réutilisation
if (0) {
  siglePedo_id.apport.nappe <-
    which(
      CNSW.sub$SER_SPEC %in% c("B", "Do", "R", "S", "B/o", "Ma") |
        CNSW.sub$DRAINAGE %in% c("e", "f", "g") |
        CNSW.sub$MAT_TEXT %in% c("V", "W")
    )
  indexPedo_id.apport.nappe <-
    dico_cnsw$INDEX_[dico_cnsw$SER_SPEC %in% c("B", "Do", "R", "S", "B/o", "Ma") |
                       dico_cnsw$DRAINAGE %in% c("e", "f", "g") |
                       dico_cnsw$MAT_TEXT %in% c("V", "W")]
  indexPedo_id.Pasapport.nappe <-
    dico_cnsw$INDEX_[!(
      dico_cnsw$SER_SPEC %in% c("B", "Do", "R", "S", "B/o", "Ma") |
        dico_cnsw$DRAINAGE %in% c("e", "f", "g") |
        dico_cnsw$MAT_TEXT %in% c("V", "W")
    )]
  class <-
    rbind(cbind(indexPedo_id.apport.nappe, 1),
          cbind(indexPedo_id.Pasapport.nappe, 0))
  r.apport.nappe <- classify(r.cnsw, class)
  writeRaster(r.apport.nappe, apport.nappe.path, overwrite = TRUE)
}
r.apport.nappe <- rast(apport.nappe.path)

#------------------------------------------------------

cat("Calcul des indices topographiques (TPI locac basic, TPI local pond?r? inverse des distances, TPI large, pente) \n")

MNT.resampled <- DeZoom(r.mnt, rayon.pente, method = "bilinear")
slope.raster <-
  terrain(MNT.resampled,
          v = 'slope',
          neighbors = 8,
          unit = 'degrees')

# TPI = topographic position index, alti(i)-moyenne alt(i+rayon)
# je fait un TPI avec alt moyenne invers?ment pond?r?e par la distance, afin de discriminer les zones plates localement mais globalement en fond de vall?e
# le tpi c'est le plus long en temps de calcul, je dezoom avant sinon trop long:

pond.TPI.norm <- focalMat(r.mnt, d = rayon.tpi.norm, "circle")
TPI.basic.raster <-
  r.mnt - focal(
    r.mnt,
    w = pond.TPI.norm,
    fun = "sum",
    na.rm = T,
    pad = F
  )

pond.TPI.norm <- focalMat(r.mnt, d = rayon.tpi.norm / 2, "circle")
TPI.basic2.raster <-
  r.mnt - focal(
    r.mnt,
    w = pond.TPI.norm,
    fun = "sum",
    na.rm = T,
    pad = F
  )

MNT.resampled <-
  DeZoom(r.mnt, 2 * resolution) # on descend la r?solution, car tpi ? large ?chelle
if(0){
# tpi inv grand rayon
pond.TPI <- focalMat(MNT.resampled, d = rayon.tpi.inv, "Gauss")
# enlever les valeurs les plus basses (coins) - warning, pour les enlever je dois mettre une valeur élevée!
#pond.TPI[pond.TPI<quantile(pond.TPI,0.25)]<-quantile(pond.TPI,0.35)
pond.TPI[pond.TPI < quantile(pond.TPI, 0.25)] <- 0
# inverser la pond?ration
pond.TPI.inv <- 1 / pond.TPI
pond.TPI.inv[is.infinite(pond.TPI.inv)] <- 0
pond.TPI.inv <- pond.TPI.inv / sum(pond.TPI.inv)
TPI.inv.raster <-
  MNT.resampled - focal(
    MNT.resampled,
    w = pond.TPI.inv,
    fun = "sum",
    na.rm = T,
    pad = F
  )
}

#writeRaster(TPI.inv.raster,filename = "TPIinvRaster.tif",overwrite = TRUE)
TPI.inv.raster <- rast("TPIinvRaster.tif")

# je part d'un filtre gaussien que je modifie
# tpi petit rayon, mais en pond?ration inverse ?galement - test
pond.TPI <- focalMat(MNT.resampled, d = rayon.tpi.norm, "Gauss")
# enlever les valeurs les plus basses (coins)
pond.TPI[pond.TPI < quantile(pond.TPI, 0.25)] <- 0
# inverser la pond?ration
pond.TPI.inv <- 1 / pond.TPI
pond.TPI.inv[is.infinite(pond.TPI.inv)] <- 0
pond.TPI.inv <- pond.TPI.inv / sum(pond.TPI.inv)
TPI.norm.raster <-
  MNT.resampled - focal(
    MNT.resampled,
    w = pond.TPI.inv,
    fun = "sum",
    na.rm = T,
    pad = F
  )


cat("D?tection des zones d'apport quasi permanent (r?seau hydro + topographie) \n")

# zut les raster ne sont pas alignée - r.cnsw/r.ai d'une part et r.mnt d'autre part...
r.mnt.res <- resample(r.mnt, r.ae)
MNT.hydro <- ifel(!is.na(hydro.r), r.mnt.res, NA)

# it?ration de calcul de moyenne avec focale faire grandir la r?gion autour du r?seau hydro

window <- focalMat(MNT.hydro, d = 2 * xres(MNT.hydro), "rectangle")
window[] <- 1
for (i in 1:3) {
  test <- focal(
    MNT.hydro,
    w = window,
    fun = mean,
    na.rm = T,
    pad = F
  )
  MNT.hydro <-
    ifel(is.na(MNT.hydro) & !is.na(test), test, MNT.hydro)
  cat("--")
}

# c'est donc un mnt relatif ? la hauteur du niveau de l'eau, valable localement autour des ruisseaux et axes hydriques
crs(r.mnt.res) <- crs(MNT.hydro)
mnt.rel <- r.mnt.res - MNT.hydro

#writeRaster(mnt.rel, "mntRel3.tif", overwrite=TRUE)

# toutes les couches doivent avoir la m?me r?solution et le meme extend (attention, je n'ai pas mis le bilinear, plus rapide mais moins pr?cis)
cat("pr?paration des raster\n")
TPI.basic.raster <- resample(TPI.basic.raster, r.ae)
TPI.basic2.raster <- resample(TPI.basic2.raster, r.ae)
TPI.norm.raster <- resample(TPI.norm.raster, r.ae)
TPI.inv.raster <- resample(TPI.inv.raster, r.ae)
slope.raster <- resample(slope.raster, r.ae)

cat("Cr?ation de la carte d'apport d'eau \n")


# les seuillages - attention, package raster démarre ses classes à 1, terra démarre à 0! mais en fait je n'utilise pas toute ces classes de seuillage dans la suite..

#class.tpi.basic <-  classify(TPI.basic.raster, c(-Inf, -4, 0.5, +Inf))# classe 0; 1 et 2
class.tpi.basic2 <-
  classify(TPI.basic2.raster, c(-Inf, 3, 5, +Inf)) # pour d?tecter cr?tes (perte en eau) : rayon "petit"
class.tpi.basic3 <-
  classify(TPI.basic.raster, c(-Inf, 3, +Inf))  # pour d?tecter cr?tes (perte en eau) : rayon "moyen"
class.tpi.norm <-
  classify(TPI.norm.raster, c(-Inf, seuil.tpi, +Inf)) # vallon encaiss? .  rayon de 75 en gauss,fonctionne bien avec seuil de -5. bon, vu que je le combine maintenant avec le tpi basic, je remonte le seuil ? -2
class.tpi.inv <- classify(TPI.inv.raster, c(-Inf, -12, +Inf)) # en 2019, seuil à -10, en 2022, je met à -12
class.slope <- classify(slope.raster, c(0, 3, +Inf))
#class.mnt.rel <-  classify(mnt.rel, c(-Inf, 1.5, +Inf))# je peux me montrer s?v?re car concerne par d?finition les petits cours d'eau - al?a inondation prend le reste

# arbre de d?cision/classification
# attention, donner des num?ro de classif qui soient en gradation avec les classes d'apport, sinon lors du resample il moyenne des pixels et change de classe de mani?re anormale

# apport quati constant al?a inondation
r.ae <-ifel(r.ai > 1,                                        1, r.ae)

# version 2019 ; j'utilisais les seuils de 2 m et 3 m pour le mnt rel, je dois descendre car trop élevé. j'essaie 1m et 2 m
r.ae <-
  ifel(r.ae == 0 & mnt.rel < 1 & r.buf.hydro.perm == 1,     1, r.ae)
r.ae <-
  ifel(r.ae == 0 & r.ai == 1,                                        2, r.ae) #alea 1 = très faible probabilité
r.ae <-
  ifel(r.ae == 0 & mnt.rel < 2 & r.buf.hydro.perm == 1,     2, r.ae)
r.ae <-
  ifel(r.ae == 0 &
         mnt.rel < 2 &
         r.buf.hydro.tempo == 1,     2, r.ae)   # apport variable car ?coulement variable

# zone de fond de vall?e large en zone peu pentue (pas toujour enti?rement d?tect? autour du r?seau hydro, car grande surface, ni dans Alea inondation)
r.ae <-
  ifel(r.ae == 0 & class.tpi.inv == 0 & class.slope == 0,     3, r.ae)

# apport permanent depuis une nappe phr?atique (sur base topo, l? ou la p?do n'est plus capable de voir sous le sol)
r.ae <- ifel(r.ae == 0 &  r.apport.nappe == 1,     3, r.ae)

# apport lat?raux non permanent, uniquement sur base topo

r.ae <-
  ifel(r.ae == 0 &
         TPI.norm.raster < (-10) & TPI.basic.raster < (-5),     4, r.ae)
r.ae <-
  ifel(r.ae == 0 &
         TPI.norm.raster < (-15) & TPI.basic.raster < (-2),     5, r.ae)

# sans apport : le reste (la majorit? en fait)
r.ae <- ifel(r.ae == 0,     6, r.ae)

# perte en eau - class tpi basic, rayon moyen, tpi basic 2, rayon plus restreint
r.ae <-
  ifel(class.tpi.basic2 == 2,  7, r.ae) # tr?s sur?lev? localement
r.ae <-
  ifel(class.tpi.basic2 == 1 &
         class.tpi.basic3 == 1,  7, r.ae) # moins sur?lev? localement mais tout de m?me sur?lev? globalement

writeRaster(r.ae,
            filename = paste0(filename, "_META_CLASSIF.tif"),
            overwrite = T)

r.ae.reclass <-
  classify(r.ae,
           rbind(
             cbind(1, 3) ,
             cbind(2, 2) ,
             cbind(3, 2)   ,
             cbind(4, 2) ,
             cbind(5, 2),
             cbind(6, 1) ,
             cbind(7, 1)
           ))

writeRaster(r.ae.reclass,
            filename = paste0(filename, ".tif"),
            overwrite = TRUE)
if (quietly == F) {
  cat(
    paste(
      "fin carto des apport d'eau (Sevrin Damien 2008 + Fran?ois Ridremont 2016 + Jo lisein 2017, 2019 et 2022). \nVoir le Fichier ",
      filename,
      ".tif qui contient les r?sultats. 1=sans apport, 2= apport non permanent, 3=apport permanent",
      sep = ""
    )
  )
}
