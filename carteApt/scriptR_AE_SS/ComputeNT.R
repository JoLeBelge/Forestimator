# 01/2017

# 06/2020 : changement de la carte, 
# 1 je recalcule la carte des pH avec une meilleure résolution (de 10 mètres, comme toutes les cartes FEE) --> même méthodologie que FR
# 2 -clé NT ; sigle série spéciale "B", "R", "S", "J" --> NT 0. Ca regle le problème de la carte des pH qui est nodata pour PTS 10 000

# il y a un bug qui a été corrigé lors du calcul de la version 2 de cette carte ; le fait que la ligne NT[is.na(pH[])] <- 0 était en fin de clé effacais les sols tourbeux de haute fagne pour lequel il n'y avais pas de donnée de pH (mais on s'en fout du pH, tourbe = -3)
# une autre différence c'est les sols à substrat i (calcaire) en lorraine qui passent en +1 et qui étaient en 0 avant.

gClip <- function(shp, bb){
  if(class(bb) == "matrix") b_poly <- as(extent(as.vector(t(bb))), "SpatialPolygons")
  else b_poly <- as(extent(bb), "SpatialPolygons")
  gIntersection(shp, b_poly, byid = T)
}

open_cnsw_tuiles <- function(zone)
{
  tuiles.shp.path <- "/home/lisein/virtualBoxSharing/NH_RW2019/data/CNSW_tuiles//grille50x50km.shp"
  tuiles <- readShapePoly(tuiles.shp.path)
  zone <- gUnaryUnion(zone)
  # determiner dans quelle tuiles se situe la zone
  #
  tuile<- tuiles[which(gContains(tuiles,gBuffer(zone,width=-10), byid=T)),] # c(which(gOverlaps(zone, tuiles, byid=T)),
  #if(length(tuile)==0){ tuile <- tuiles[which(gOverlaps(tuiles,zone, byid=T)),]} retourne 2 dalle si la zone touche 2, ?a foire; je fait un buf n?gatif, voir ligne au dessus
  cnsw.tuile.name <- paste("/home/lisein/virtualBoxSharing/NH_RW2019/data/CNSW_tuiles//CNSW_tuile",tuile$ID,".shp",sep="")
  rds.name <- paste("/home/lisein/virtualBoxSharing/NH_RW2019/data/CNSW_tuiles//CNSW_tuile",tuile$ID,".rds",sep="")
  
  if (file.exists(rds.name)){ cnsw <- readRDS(rds.name)
  return(cnsw)
  }else{
    
    if (file.exists(cnsw.tuile.name)){ cnsw <- readShapePoly(cnsw.tuile.name)
    saveRDS(cnsw, file=rds.name)
    }else{cat(paste("le fichier ", cnsw.tuile.name, " n'existe pas"))}
  }
  
  return(cnsw)
}

# g?re l'?ventualit? ou aucun polygons n'est pr?sent -- retourne une carte vide - et fait le rasterize en parrall?le, tellement lent
ToRaster <- function(polygons,raster){ 
  
  if(length(polygons)!=0){
    
    #myRaster <- foreach(i= 1:nsc, .packages="raster", .combine=c) %dopar% {
    #  rasterize(polygons, raster,background=0,field=1)
    #}  
    myRaster <- rasterize(polygons, raster,background=0,field=1)
    #myRaster <- rasterize(polygons,raster,field=1,background=0)
  } else {
    myRaster <-raster
    myRaster[] <- 0
  }
  return(myRaster)
}

# en suivant simplement la cl? abiotique avec la carte des pH moyen de FR
# ajout remarques de Hc du 10/02/2017 : tourbes en ardenne: -3 -> Tourbe Hors Ardenne --> Indetermin?
# +2: Gbb k, K, kf phase de profondeur 6 uniquement
# ajout remarques de HC du 14/02/2017 : ardenne condruzienne (territoire eco): d?classer les PTS 4000 (peu importe drainage) de -1 ? -2, car se sont des placages limoneux mais de faible profondeurs et sur le m?me substrat que le socle ardennairs== acide
# limon (? nouveau PTS 4010 et 4020) en r?gion limoneuse : mettre en -1 et non pas en -2 comme le fait le pH
map_NT_cleAbiotique <- function (shp.polyg.path="D://Lisein_j//AC2016//RelFloristique//Jo//ApportEau_et_NH//shp//zoneTestNeupont2.shp",
                                 filename="",
                                 pH.path = "/home/lisein/Documents/Carto/pH/2020/cartepH2020.tif",
                                 Zbio.raster.path =  "/home/lisein/Documents/Carto/FEEW2020/ZBIO.tif",
                                 terEco.raster.path =  "/home/lisein/Documents/Lisein_j/AC2016/FEE/shp/TECO.tif",
                                # terEco.path =  "D://Lisein_j//AC2016//FEE//FEE_rev//Data rubriques//Aptitude//carte_bioclimatique//territoires_ecologiques",
                                 lim.m32 = 3.8,
                                 lim.m12 = 4.2,
                                 lim.p12 = 7.5
)
{
  ptm <- proc.time()[3]
  cat("G?n?ration de la carte des niveaux trophiques (avec carte pH moyen)...\n")
  
  # la zone d'?tude
  zone <- readShapePoly(shp.polyg.path)
  
  pH <- raster(pH.path)
  # CNSW
  CNSW <- open_cnsw_tuiles(zone=zone)
  
  #Zbio.shp <- readShapePoly(Zbio.path)
  #terEco.shp <- readShapePoly(terEco.path)
  
  # on clippe toutes les couches lourdes
  pH <-  crop(pH, zone)
  #pH[pH[]==9999] <- NA

  pH[pH[]==1] <- NA # si il n'a pas trouvé de pH pour ce PTS / RegNat, valeur de 1
  pH[pH[]==0] <- NA # si pas de PTS (hors wallonie par exemple), valeur de 0
 
  sub <- gIntersects(zone, CNSW, byid = TRUE)
  CNSW.sub <<- CNSW[which(sub),]
  
  #ZBIO <- rasterize(Zbio.shp, pH,background=0,field=Zbio.shp$Zbio)
  # 2020 ; pour gagner un peu de temps de calcul, je génère avec gdal les raster pour zbio et teco
  ZBIO <- raster(Zbio.raster.path)
  ZBIO <- crop(ZBIO, zone) 
  
  #TECO <- rasterize(terEco.shp, pH,background=0,field=terEco.shp$N_SECT_1)
  
  TECO <- raster(terEco.raster.path)
  TECO <- crop(TECO, zone) 
  
  # d?termination de SIG_alluvion, calcaire, profond, superficiel depuis cnsw
  select.sol <- which(CNSW.sub$SUBSTRAT %in% c("j", "k", "m", "n", "i", "kf", "j-w", "ks", "kt", "(+u)") | CNSW.sub$PHASE_2 %in% c("(ca)","(k)") | CNSW.sub$PHASE_6 %in% c("J","M") | CNSW.sub$CHARGE %in% c("k", "K", "kf", "m", "n"))
  calcaire <- ToRaster(CNSW.sub[select.sol,], pH)
  
  # modif 2020
  serie.special.riche <- ToRaster(CNSW[which(CNSW.sub$SER_SPEC %in% c("B", "R", "S", "J")),], pH)

  select.sol <- which(CNSW.sub$PHASE_1 %in% c("0", "1", "0_1", "0_1_2") | CNSW.sub$PHASE_2 %in% c("0", "1", "0_1", "0_1_2") | (is.na(CNSW.sub$PHASE_1) & is.na(CNSW.sub$PHASE_2)))
  profond <- ToRaster(CNSW.sub[select.sol,], pH)
  
  select.sol <- which(CNSW.sub$DEV_PROFIL %in% c("p", "P"))
  alluvion <- ToRaster(CNSW.sub[select.sol,], pH)
  
  select.sol <- which(CNSW.sub$DEV_PROFIL %in% c("g"))
  podzol <- ToRaster(CNSW.sub[select.sol,], pH)
  
  select.sol <- which(CNSW.sub$DEV_PROFIL %in% c("c", "f"))
  podzolique <- ToRaster(CNSW.sub[select.sol,], pH)
  
  select.sol <- which(CNSW.sub$PHASE_1 %in% c("6") | CNSW.sub$PHASE_2 %in% c("6"))
  superficiel <- ToRaster(CNSW.sub[select.sol,], pH)
  
  select.sol <- which(CNSW.sub$MAT_TEXT %in%  c("V", "W", "V-E") | CNSW.sub$PHASE_4 %in% c("(v)","(v3)", "(v4)"))
  tourbe <- ToRaster(CNSW.sub[select.sol,], pH)
  
  # Ardenne Condruzienne
  dico <- cbind(c("G","A","L","E","A-G","S","(G)","Z","U","P","V","G-L","W","A-L","S-Z","A-S","U-L-S","A-G-S","A-E","E-L-S","A-U","S-U","V-E","U-L","L-E","E-Z","S-G","A-S-U","G-Z"),c("G","A","L","E","G","S","G","Z","U","P","V","G","W","A","S","S","S","S","A","S","A","S","V","U","L","Z","G","S","G"))
  select.sol <- which(CNSW.sub$MAT_TEXT %in%  dico[dico[,2] %in% c("A"),1] )
  limon <- ToRaster(CNSW.sub[select.sol,], pH)
  
  # application de la cl?
  NT <- pH
  NT[] <- 0
  
  NT[pH[]>=lim.p12 ] <- 12
  
  NT[calcaire[]==1 & pH[]<lim.p12  & (podzol[]==1 | podzolique[]==1)] <- 9
  NT[calcaire[]==1 & pH[]<lim.p12  & (podzol[]==0 & podzolique[]==0) & pH[] <5 & profond[]==0] <- 9
  # 9 : comme la cl? actuelle.
  NT[calcaire[]==1 & pH[]<lim.p12  & (podzol[]==0 & podzolique[]==0) & pH[] <5 & profond[]==1] <- 9
  
  NT[calcaire[]==1 & pH[]<lim.p12  & (podzol[]==0 & podzolique[]==0) & pH[] >=5 & profond[]==1] <- 10
  NT[calcaire[]==1 & pH[]<lim.p12  & (podzol[]==0 & podzolique[]==0) & pH[] >=5 & profond[]==0] <- 11
  
  NT[calcaire[]==1 & superficiel[]==1 & pH[]<lim.p12  & (podzol[]==0 & podzolique[]==0)] <- 12
  
  # calcaire non d?tect?
  NT[(calcaire[]==0  & pH[]<lim.p12) & alluvion[]==1] <- 10
  NT[(calcaire[]==0  & pH[]<lim.p12) & podzolique[]==1] <- 8
  NT[(calcaire[]==0  & pH[]<lim.p12) & podzol[]==1] <- 7
  
  NT[(calcaire[]==0 & pH[]<lim.p12) & (podzolique[]==0 & podzol[]==0 & alluvion[]==0)  & pH[] >=5] <- 10
  NT[(calcaire[]==0 & pH[]<lim.p12) & (podzolique[]==0 & podzol[]==0 & alluvion[]==0)  & pH[] <5 & pH[] >lim.m12] <- 9
  NT[(calcaire[]==0 & pH[]<lim.p12) & (podzolique[]==0 & podzol[]==0 & alluvion[]==0)  & pH[] <=lim.m12 & pH[] >lim.m32] <- 8
  NT[(calcaire[]==0 & pH[]<lim.p12) & (podzolique[]==0 & podzol[]==0 & alluvion[]==0)  & pH[] <=lim.m32] <- 7
  NT[pH[] <lim.m32] <- 7
  
  # limon en ardenne condruzienne
  NT[limon[]==1 & TECO[] %in% c(14) & calcaire[]==0  & alluvion[]==0  & podzol[]==0 ] <- 8
  # limon en r?gion limoneuse
  NT[limon[]==1 & ZBIO[] %in% c(6,7) & calcaire[]==0  & alluvion[]==0  & podzol[]==0 ] <- 9
  
  # attention, si na sur carte pH, attribue mauvaise classe trophique.
  
  # en 2017 je mettais cette ligne de code en fin de clé, du coup certains sols en zone de tourbe qui n'avait pas de valeur de pH se voyais attribuer un nodata.
  # corrigé en 2020
  NT[is.na(pH[])] <- 0
  
  # ajout Claessens 10/02: tourbe
  # en Ardenne
  NT[tourbe[]==1 & ZBIO[] %in% c(1,2,10)] <- 7
  # hors ardenne : indetermin?
  NT[tourbe[]==1 & !ZBIO[] %in% c(1,2,10)] <- NA
  
  NT[serie.special.riche[]==1] <- 10
  
  if (filename!=""){
    writeRaster(NT, filename=filename, format="GTiff",overwrite=T)
  }
  
  cat(paste("total time : " ,round((proc.time()[3] - ptm)/60,0)," minutes\n"))
}


# finalement HC aime bien la carte avec 4.2 partout, meme si il y a une large zone de -1 en ardenne, cela semble coh?rent pour lui

# non j'ai pas utilisé cléAbiotique 2 en 2017.
map_NT_cleAbiotique2 <- function (shp.polyg.path="D://Lisein_j//AC2016//RelFloristique//Jo//ApportEau_et_NH//shp//zoneTestNeupont2.shp",
                                 filename="",
                                 pH.path = "/home/lisein/Documents/Carto/pH/2020/cartepH2020.tif",
                                 Zbio.raster.path =  "/home/lisein/Documents/Carto/FEEW2020/ZBIO.tif",
                                 lim.m32 = 3.8,
                                 lim.m12 = 4.2,
                                 lim.m12Ardenne = 4.5,
                                 lim.p12 = 7.5
)
{
  ptm <- proc.time()[3]
  cat("G?n?ration de la carte des niveaux trophiques (avec carte pH moyen)...\n")
  
  # la zone d'?tude
  zone <- readShapePoly(shp.polyg.path)
  cat("zone chargée\n")
  
  pH <- raster(pH.path)
  cat("carte pH chargée\n")
  # CNSW
  CNSW <- open_cnsw_tuiles(zone=zone)
  
  cat("cnsw chargée\n")
  
  #Zbio.shp <- readShapePoly(Zbio.path)
  
  # on clippe toutes les couches lourdes
  pH <-  crop(pH, zone)
  pH[pH[]==1] <- NA
  sub <- gIntersects(zone, CNSW, byid = TRUE)
  CNSW.sub <<- CNSW[which(sub),]
  
  #ZBIO <- rasterize(Zbio.shp, pH,background=0,field=Zbio.shp$Zbio)
  
  ZBIO <- raster(Zbio.raster.path)
  ZBIO <- crop(ZBIO, zone)
  
  # d?termination de SIG_alluvion, calcaire, profond, superficiel depuis cnsw
  select.sol <- which(CNSW.sub$SUBSTRAT %in% c("j", "k", "m", "n", "i", "kf", "j-w", "ks", "kt", "(+u)") | CNSW.sub$PHASE_2 %in% c("(ca)","(k)") | CNSW.sub$PHASE_6 %in% c("J","M") | CNSW.sub$CHARGE %in% c("k", "K", "kf", "m", "n"))
  calcaire <- ToRaster(CNSW.sub[select.sol,], pH)
  
  select.sol <- which(CNSW.sub$PHASE_1 %in% c("0", "1", "0_1", "0_1_2") | CNSW.sub$PHASE_2 %in% c("0", "1", "0_1", "0_1_2") | (is.na(CNSW.sub$PHASE_1) & is.na(CNSW.sub$PHASE_2)))
  profond <- ToRaster(CNSW.sub[select.sol,], pH)
  
  select.sol <- which(CNSW.sub$DEV_PROFIL %in% c("p", "P"))
  alluvion <- ToRaster(CNSW.sub[select.sol,], pH)
  
  select.sol <- which(CNSW.sub$DEV_PROFIL %in% c("g"))
  podzol <- ToRaster(CNSW.sub[select.sol,], pH)
  
  select.sol <- which(CNSW.sub$DEV_PROFIL %in% c("c", "f"))
  podzolique <- ToRaster(CNSW.sub[select.sol,], pH)
  
  select.sol <- which(CNSW.sub$PHASE_1 %in% c("6") | CNSW.sub$PHASE_2 %in% c("6"))
  superficiel <- ToRaster(CNSW.sub[select.sol,], pH)
  
  select.sol <- which(CNSW.sub$MAT_TEXT %in%  c("V", "W", "V-E") | CNSW.sub$PHASE_4 %in% c("(v)","(v3)", "(v4)"))
  tourbe <- ToRaster(CNSW.sub[select.sol,], pH)
  
  # modif 2020
  serie.special.riche <- ToRaster(CNSW[which(CNSW.sub$SER_SPEC %in% c("B", "R", "S", "J")),], pH)
  
  # application de la cl?
  NT <- pH
  NT[] <- 0
  
  NT[pH[]>=lim.p12 ] <- 12
  
  NT[calcaire[]==1 & pH[]<lim.p12  & (podzol[]==1 | podzolique[]==1)] <- 9
  NT[calcaire[]==1 & pH[]<lim.p12  & (podzol[]==0 & podzolique[]==0) & pH[] <5 & profond[]==0] <- 9
  # 9 : comme la cl? actuelle.
  NT[calcaire[]==1 & pH[]<lim.p12  & (podzol[]==0 & podzolique[]==0) & pH[] <5 & profond[]==1] <- 9
  
  NT[calcaire[]==1 & pH[]<lim.p12  & (podzol[]==0 & podzolique[]==0) & pH[] >=5 & profond[]==1] <- 10
  NT[calcaire[]==1 & pH[]<lim.p12  & (podzol[]==0 & podzolique[]==0) & pH[] >=5 & profond[]==0] <- 11
  
  NT[calcaire[]==1 & superficiel[]==1 & pH[]<lim.p12  & (podzol[]==0 & podzolique[]==0)] <- 12
  
  # calcaire non d?tect?
  NT[(calcaire[]==0  & pH[]<lim.p12) & alluvion[]==1] <- 10
  NT[(calcaire[]==0  & pH[]<lim.p12) & podzolique[]==1] <- 8
  NT[(calcaire[]==0  & pH[]<lim.p12) & podzol[]==1] <- 7
  
  NT[(calcaire[]==0 & pH[]<lim.p12) & (podzolique[]==0 & podzol[]==0 & alluvion[]==0)  & pH[] >=5] <- 10
  NT[(calcaire[]==0 & pH[]<lim.p12) & (podzolique[]==0 & podzol[]==0 & alluvion[]==0)  & pH[] <5 & pH[] >lim.m12 ] <- 9
  NT[(calcaire[]==0 & pH[]<lim.p12) & (podzolique[]==0 & podzol[]==0 & alluvion[]==0)  & pH[] <=lim.m12 & pH[] >lim.m32] <- 8
  #NT[(calcaire[]==0 & pH[]<lim.p12) & (podzolique[]==0 & podzol[]==0 & alluvion[]==0)  & pH[] <=lim.m32] <- 7
  
  # test - j'ai remarqu? que la limite de pH de 4.2 me donne de bon r?sulats: moins de stations en -2 hors ardenne, je pense au condroz par ex ou la limite de 4.5 range les sol ? charges psammitiques en -2 alors qu'il doivent ?tre en -1
  # inconv?nient de la limite 4.2 : en Ardenne, plein de sol tombent alors en -1, c'est louche. Solution: adapter la limite de pH ? la zone bioclimatique 
  NT[(calcaire[]==0 & pH[]<lim.p12) & (podzolique[]==0 & podzol[]==0 & alluvion[]==0)  & pH[] <5 & pH[] >lim.m12Ardenne & ZBIO[] %in% c(1,2,10)] <- 9
  NT[(calcaire[]==0 & pH[]<lim.p12) & (podzolique[]==0 & podzol[]==0 & alluvion[]==0)  & pH[] <=lim.m12Ardenne & pH[] >lim.m32 & ZBIO[] %in% c(1,2,10)] <- 8
  
  NT[pH[] <lim.m32] <- 7
  
  NT[is.na(pH[])] <- 0
  
  # ajout Claessens 10/02: tourbe
  # en Ardenne
  NT[tourbe[]==1 & ZBIO[] %in% c(1,2,10)] <- 7
  # hors ardenne : indetermin?
  NT[tourbe[]==1 & !ZBIO[] %in% c(1,2,10)] <- NA
  
  NT[serie.special.riche[]==1] <- 10
  
  # attention, si na sur carte pH, attribue mauvaise classe trophique.
  
  if (filename!=""){
    writeRaster(NT, filename=filename, format="GTiff",overwrite=T)
  }
  
  cat(paste("total time : " ,round((proc.time()[3] - ptm)/60,0)," minutes\n"))
}


# en fait la classif RF des NT je sais pas trop laquelle je vais utiliser, par contre pr?parer les shp avant la classif c'est long, je le fait donc en 2 parties
# PAS n?cessaire, Fran?ois R l'avais d?j? fait! PTS + cnsw + regnat

if (0){
load("D://Lisein_j//AC2016//RelFloristique//Jo//Phyto_NH_NT//modeleRF//rfNTabioREGNAT42.R")
load("D://Lisein_j//AC2016//RelFloristique//Jo//Phyto_NH_NT//modeleRF//rfNTabiopHonlyTECOlim42.R")

map_NT_applyRF <- function (
                          filename=""
)
{
  ptm <- proc.time()[3]
  cat("G?n?ration de la carte des niveaux trophiques...\n")
 
  # je n'ai pas besoin de la carte des pH, mais j'ai besoin d'un raster "d'exemple" pour rasterizer les shp Zbio et RegNat --> plus rapide
  #pH.path = "D://Carto//pH//pcnsw_regnat5_carte_pH.tif"
  #pH <- raster(pH.path)
 
  path.dbf <- "D://Carto//pH//backup/pcnsw_regnat5.dbf"
 
  require(foreign)
  df <- read.dbf(path.dbf)
  
  #save(list=c("shp") , file ="modeleRF//cnswxptsxregnat.R")

  # d?termination de SIG_alluvion, calcaire, profond, superficiel depuis cnsw
  select.sol <- which(df$SUBSTRAT %in% c("j", "k", "m", "n", "i", "kf", "j-w", "ks", "kt", "(+u)") | df$PHASE_2 %in% c("(ca)","(k)") | df$PHASE_6 %in% c("J","M") | df$CHARGE %in% c("k", "K", "kf", "m", "n"))
  #calcaire <- ToRaster(CNSW.sub[select.sol,], pH)
  df$SIG_calcaire <- 0
  df$SIG_calcaire[select.sol]<- 1
  
  select.sol <- which(df$PHASE_1 %in% c("0", "1", "0_1", "0_1_2") | df$PHASE_2 %in% c("0", "1", "0_1", "0_1_2") | (is.na(df$PHASE_1) & is.na(df$PHASE_2)))
  #profond <- ToRaster(CNSW.sub[select.sol,], pH)
  
  df$SIG_profond <- 0
  df$SIG_profond[select.sol]<- 1
  
  select.sol <- which(df$PHASE_1 %in% c("4", "5","6", "7") | df$PHASE_2 %in% c("4", "5","6", "7"))
  select.sol <- which(df$PHASE_1 %in% c("4", "5","6", "7") | df$PHASE_2 %in% c("4", "5","6", "7"))
  #superficiel <- ToRaster(CNSW.sub[select.sol,], pH)
  df$SIG_superficiel <- 0
  df$SIG_superficiel[select.sol]<- 1
  
  select.sol <- which(df$DEV_PROFIL %in% c("p", "P"))
  #alluvion <- ToRaster(CNSW.sub[select.sol,], pH)
  df$SIG_alluvion <- 0
  df$SIG_alluvion[select.sol]<- 1
  
  select.sol <- which(df$DEV_PROFIL %in% c("g"))
  #podzol <- ToRaster(CNSW.sub[select.sol,], pH)
  df$SIG_podzol <- 0
  df$SIG_podzol[select.sol]<- 1
  
  select.sol <- which(df$DEV_PROFIL %in% c("c", "f"))
  #podzolique <- ToRaster(CNSW.sub[select.sol,], pH)
  df$SIG_podzolique <- 0
  df$SIG_podzolique[select.sol]<- 1
  
  #df$PTS2 <- as.factor(floor(df$PTS/100))
  df$REGNAT <- as.factor(df$regnat)
  
  df$PTS <- df$pts_typolo
  # charge crayeuse --> com calcaire
  #df$PTS[df$PTS==7810] <- 7510
  #df$PTS[df$PTS==1000] <- NA # non cartographi?
  df$PTS2 <- as.factor(floor(df$PTS/100))
  df$PTS <- as.factor(df$PTS)
  df$PTS <- droplevels(df$PTS)

  #levels(df$PTS2)== levels(sites2$PTS2)
  
  #df[1,var.inter2]
  
  # lim 42 avec s?lection des NT avec vote
  # avec cutoff pour mettre plus de sol en niveau 1, genre les argiles lourdes sur substrat calcaire de gaume qui sans cutoff sont class?e en 0
  t <- 1+1+1+0.6 # la class -1 prend trop de place du coup
  my.co <- c(1/t,1/t,1/t,0.6/t)
  NT <- predict(rf.abio42, na.roughfix(df[,var.inter2]), cutoff=my.co)
  
  NT <- predict(rf.abio42, na.roughfix(df[,var.inter2]))
  #NT <- predict(rf.abio45, na.roughfix(df[,var.inter2]))
  #CNSW.sub@data$NT <- as.integer(NT)-4+10
  # pr?diction seulement de -2,-1 0 et 1
  df$NT <- as.integer(NT)-3+10
  
  for (nt in 8:11){pc <- round(100*sum(df$Shape_Area[df$NT==nt])/sum(df$Shape_Area))
  cat(paste0("niveau trophique ", nt-10, " :" ,pc, " %\n" ))}
  
  # compl?ment? avec la cl? abiotique
  df$NT[df$SIG_calcaire==1 & df$SIG_superficiel==1 & (df$SIG_podzol==0 & df$SIG_podzolique==0)] <- 12
  df$NT[df$SIG_calcaire==1 & df$SIG_profond==1 & (df$SIG_podzol==0 & df$SIG_podzolique==0)] <- 10
  df$NT[df$SIG_calcaire==1 & (df$SIG_podzol==1 | df$SIG_podzolique==1)] <- 9
  
  # calcaire non d?tect?
  df$NT[df$SIG_calcaire[]==0  & df$SIG_alluvion[]==1] <- 10
  df$NT[df$SIG_calcaire[]==0  & df$SIG_podzolique[]==1] <- 8
  df$NT[df$SIG_calcaire[]==0  & df$SIG_podzol[]==1] <- 7
  
  for (nt in 7:12){pc <- round(100*sum(df$Shape_Area[df$NT==nt])/sum(df$Shape_Area))
  cat(paste0("niveau trophique ", nt-10, " :" ,pc, " %\n" ))}
  
  filename <- "D://Carto//pH//pcnsw_regnat5.dbf"
  
  write.dbf(df[,c("SIGLE_PEDO","PTS","REGNAT","PTS2", "SIG_calcaire", "SIG_superficiel","NT")],filename, factor2char = TRUE)
  

  cat(paste("total time : " ,round((proc.time()[3] - ptm)/60,0)," minutes\n"))
}
}


# ?a je pourrais largement remplacer par quelquechose de plus rapide, tels que un poly to point puis un extract d'un raster
# centre <- gCentroid(CNSW.sub, byid=T)

