# juin 2016: calculer les niveaux hydriques et trophique, sous SIG et en combinaison avec observation terrain (pH Labo par ex)

# charger mes fonctions maison
require(raster)
#library(rgeos)
require(maptools)
require(rgdal)

setwd("/home/lisein/virtualBoxSharing/NH_RW2019")

source("script/ApportEau_SousSecteur.R")


# sans l'extention tif
fileAE="/home/lisein/Documents/Carto/FEEW2020/AE_W_201911"
fileSS="/home/lisein/Documents/Carto/FEEW2020/SS_W_201911"


# test et d?bug
shp.polyg.path <- "data//test//zoneTestNeupont.shp"
shp.polyg.path <- "data//test//zoneTestEscaut.shp"
inondation.path <- "I://NH_GDL2019//data//alea_inondation_20070626//alea_inondation_20070626.shp"
resolution<- 10  
MNT.path <- "data/MNT_10m_WALLONIA.tif"
rayon.pente<-80 # +- hauteur d'un arbre
rayon.exposition <-50 # terrain de foot
rayon.tpi.inv <- 200
rayon.tpi.norm <- 75
filename <- "NH_neupontv1"
writeRaster(TPI.basic.raster, filename="tpibasicLibin", format="GTiff",overwrite=T)

writeRaster(AE.raster.resampled.reclass, filename="testAE", format="GTiff",overwrite=T)

# teste sur les zones ou j'ai scann? les vielles cartes terrain
map_ApportEau(shp.polyg.path="data//test//zoneTestLibin.shp",filename="Libin_AE5")

map_NH(shp.polyg.path="data//test//zoneTestNeupont.shp",filename="Neupont_NH1",fileAE=fileAE,fileSS=fileSS)

map_SousSecteur(shp.polyg.path="D://Lisein_j//AC2016//RelFloristique//Jo//shp//zoneTestNeupont.shp",filename="Neupont_SS2")
map_ApportEau(shp.polyg.path="D://Lisein_j//AC2016//RelFloristique//Jo//shp//zoneTestNeupont.shp",filename="Neupont_AE2")
map_NH(shp.polyg.path="D://Lisein_j//AC2016//RelFloristique//Jo//shp//zoneTestNeupont2.shp",filename="neupont_NH1")
map_NH(shp.polyg.path="D://Lisein_j//AC2016//RelFloristique//Jo//ValidationAE_NH//erable//zone_herbeumont.shp",filename="herbeumont_NH1")


map_topo(shp.polyg.path,resolution=10,quietly=F,filename="test_topoEscaut")

# carte pour 4 zones ?tudes;
fileNames <- Sys.glob("D://Lisein_j//AC2016//RelFloristique//Jo//shp//*.shp")
for (testZone in fileNames[2:4]){
  map_ApportEau(shp.polyg.path=testZone,filename=testZone,resolution=20)
}


# cartes en plain, RW. attention au messages de  Error: cannot allocate vector of size 106.0 Mb. Je dois faire tourner le script en R64bit comme ?a j'ai plus de 4Gb de m?moire vive et pas de souci de ce cot?.
tuiles <-readShapePoly( "/home/lisein/virtualBoxSharing/NH_RW2019/data/grille25kmS.shp")
for (dalle in 1:length(tuiles)){
  cat(paste("Tuile", dalle, "\n"))
  tuile <- tuiles[dalle, ]
  filetmp <- paste(getwd(),"//tmp_jo.shp",sep="")
  writePolyShape(tuile,filetmp)
  fileNH <- paste("/home/lisein/virtualBoxSharing/NH_RW2019/result/NH/NH_tuile_",tuile$ID,sep="")
  
  #fileAE <- paste("I://NH_RW2019//result//AE//AE_tuile_",tuile$ID,sep="")
  #fileSS <- paste("I://NH_RW2019//result//SS//SS_tuile_",tuile$ID,sep="")
  #fileTopo <- paste("I://NH_RW2019//result//Topo//Topo_tuile_",tuile$ID,sep="")
  #if (file.exists(paste(fileTopo,".tif",sep=""))==F){
  if (file.exists(paste(fileNH,".tif",sep=""))==F){
    # ici je dois d?finir manuellement si jamais je veux calculer l'une ou l'autres des cartes, ou toutes.
    #map_ApportEau(shp.polyg.path=filetmp,DisplayMap=F,resolution=10,quietly=F,filename=fileAE)
    map_NH(shp.polyg.path=filetmp,filename=fileNH,fileAE=fileAE,fileSS=fileSS)
    #map_topo(shp.polyg.path=filetmp,DisplayMap=F,resolution=10,quietly=F,filename=fileTopo)
    
  } else {cat(paste("Tuile", dalle, " d?j? calcul?e\n"))}
}



# compute Niveau trophique, janvier 2017, en utilisant soit un pr?dicteur for?t al?atoire, soit la carte des pH moyen, soit un m?lange de la cl? abiotique (info p?do) pour les NT -3 et 2 et d'une classif RF (NT -2,-1,0,1,)


# je refait tourner en 2020.
# je refait tourner en 2021 car Hugues trouve un bug le 22 06 2021 ; Gbbfi2 en ardenne est à NT 0 alors que Gbbfi1_2 est à -2
# les deux types de sols doivent être à -2, c'est surtout ça. Je migre tout le code vers C++

setwd("/home/lisein/Documents/Carto/NT/2021")
map_NT_cleAbiotique(shp.polyg.path="ztestPrb",filename = "NT_testEpioux")
# c'est ennuyeux, je ne parviens pas à reproduire le bug qu'il y a eu en 2020. 

source("script//ComputeNT.R")

#test
map_NT_cleAbiotique(shp.polyg.path="data//test//zoneTestNeupont",filename = "NT_testNeupont")

tuiles <-readShapePoly( "/home/lisein/virtualBoxSharing/NH_RW2019/data/grille25kmS.shp")
  # r?plication cl? abiotique avec ph de la carte des pH
  for (dalle in 1:length(tuiles)){

      cat(paste("Tuile", dalle, "\n"))
      tuile <- tuiles[dalle, ]
      filetmp <- paste(getwd(),"//tmp_jo.shp",sep="")
      writePolyShape(tuile,filetmp)
      fileNT <- paste0("/home/lisein/Documents/Carto/NT/2021/tuile/tuile_",tuile$ID)
      
      if (file.exists(paste(fileNT,".tif",sep=""))==F){
        
        map_NT_cleAbiotique(shp.polyg.path=filetmp,filename=fileNT)
        
      } else {cat(paste("Tuile", dalle, " d?j? calcul?e\n"))}
    }


# modification de la carte des NH ? post?riori

for (dalle in 1:length(tuiles)){
  tuile <- tuiles[dalle, ]
 #fileAE <- paste("D://Carto//NH//AE//AE_tuile_",tuile$ID,sep="")
  fileNH <- paste("D://Carto//NH//NH//NH_tuile_",tuile$ID,"_avecPerte.tif",sep="")
  raster <- raster(fileNH)
  raster[raster[]==6] <- 0
  raster[raster[]==15] <- 0
  
  raster[raster[]==0.5] <- 10

  raster[raster[]==11] <- 9
  raster[raster[]==12] <- 8
  raster[raster[]==13] <- 7
  raster[raster[]==14] <- 6
  raster[raster[]==1] <- 11
  raster[raster[]==2] <- 12
  raster[raster[]==3] <- 13
  raster[raster[]==4] <- 14
  raster[raster[]==5] <- 15
  
  raster[raster[]==11.5] <- 17
  raster[raster[]==12.5] <- 18
  raster[raster[]==13.5] <- 19
  
  raster[is.na(raster[])] <- 0
  
  fileN <- paste("D://Carto//NH//NH//0NH_tuile_",tuile$ID,".tif",sep="")
  writeRaster(raster, filename=fileN, format="GTiff",overwrite=T)
}

# modification de la carte des AE ? post?riori

for (dalle in 1:length(tuiles)){
  tuile <- tuiles[dalle, ]
  fileAE <- paste("D://Carto//NH//AE//AE_tuile_",tuile$ID,".tif",sep="")
  raster <- raster(fileAE)
  # j'enl?ve les pertes
  raster[raster[]==4] <- 1
  fileN <- paste("D://Carto//NH//AE//0AE_tuile_",tuile$ID,".tif",sep="")
  writeRaster(raster, filename=fileN, format="GTiff",overwrite=T)
}

