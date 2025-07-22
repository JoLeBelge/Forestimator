# 06/2016
# en entr?e des cl?s des niveaux Hydriques et Trophiques, d?termination des apports d'eau sur et des sous-secteurs sur base du MNT Lidar
# les fonctions doivent fonctionner pour un extract ponctuel et pour la g?n?ration de carte en plein (avec une r?solution pas trop fine sinon trop lent)
# remarque: grosse faiblesse sur la fenetre de pond?ration pour tpi, il faudrai tout recoder dans une fonction perso - pour l'instant le rayon est 3fois plus grand que pr?vu.

# 2019 ; autre solution pour B (-3), R (-2), F (-1) et P (prof 2) 

# 2020 ; complexe de drainage A majuscule ; n'est pas (correctement) pris en compte dans les clés NH. c'est complexe de drainage b c d--> NH de -1 (comme d). en fait je remplace juste le grand A par un drainage d

# remarque 09/2016 : la r?solution joue un role assez important, c'est sensible. toujours utiliser r?solution =10 m, sinon je crains que tout ?a ne soiet bugg?, entre autre parce que les tpi sont calcul? sur des mnt resampled (donc pas mm signification, lissage, et donc seuils moins valides
map_NH <- function (shp.polyg.path="D://Lisein_j//AC2016//RelFloristique//Jo//shp//zoneTestNeupont2.shp",
                    filename="",
                    fileAE="",
                    fileSS="",
                    res=10)
{
  ptm <- proc.time()[3]
  cat("G?n?ration de la carte des niveaux hydriques...\n")
  
  if (file.exists(paste(fileAE,".tif",sep=""))==F){
    cat("G?n?ration de la carte des apport d'eau..\n")
    AE <- map_ApportEau(shp.polyg.path=shp.polyg.path,DisplayMap=F,resolution=res,quietly=T)
  } else {
    cat(paste("chargement AE\n"))
    AE <- raster(paste(fileAE,".tif",sep=""))
  }
  
  if (file.exists(paste(fileSS,".tif",sep=""))==F){
    SS.originel <- map_SousSecteur(shp.polyg.path=shp.polyg.path,DisplayMap=F,resolution=res)
  } else {
    cat(paste("chargement SS\n"))
    SS.originel <- raster(paste(fileSS,".tif",sep=""))
  }
  
  #CNSW <- CNSW.sub
  cat("ouverture du shp tuile...\n")
  zone <- readShapePoly(shp.polyg.path)
  
  SS.originel <- crop(SS.originel, zone)
  SS <- reclassify(SS.originel, c(0,2.5,1, 2.5,3.5,2),include.lowest=T) # 1= foid ou neutre, 2 chaud
  
  
  AE <- crop(AE, zone)
  
  cat("G?n?ration de la carte des niveaux hydriques...\n")
  # manipulation des donn?es p?do pour obtenir le NH
  

 

  
  # remplacer open tuile par open le shp complet
  cat("ouverture de la tuile csnw...\n")
  CNSW <- open_cnsw_tuiles(zone=zone)
  cat("intersect...\n")
  sub <- gIntersects(zone, CNSW, byid = TRUE)
  CNSW.sub <<- CNSW[which(sub),]
  cat("done.\n")
  dico <- cbind(c("G","A","L","E","A-G","S","(G)","Z","U","P","V","G-L","W","A-L","S-Z","A-S","U-L-S","A-G-S","A-E","E-L-S","A-U","S-U","V-E","U-L","L-E","E-Z","S-G","A-S-U","G-Z"),c("G","A","L","E","G","S","G","Z","U","P","V","G","W","A","S","S","S","S","A","S","A","S","V","U","L","Z","G","S","G"))
  ph.dico <- cbind(c("0","0_1","0_1_2","1","1_2","2","2_3","2_4","3","4","6","7", "5"),c("0","0","0","1","2","2","3","4","3","4","6","2", "5"))
 
  # Sol tourbeux 
  select.sol <- which(CNSW.sub$MAT_TEXT %in%  dico[dico[,2] %in% c("V", "W"),1] | CNSW.sub$PHASE_4 %in% c("(v)","(v3)", "(v4)"))
  apport.nappe <- ToRaster(CNSW.sub[select.sol,], AE)
  pas.apport.nappe <- reclassify(apport.nappe, c(0,0.5,1, 0.5,1.5,0),include.lowest=T)
  
  # sol s?rie sp?ciale, on ne sais pas faire grand chose avec eux maleureusement - je les met de cot? avec un code sp?cial
  select.sol <- which(CNSW.sub$SER_SPEC %in% c("G-T")) # sur cl?, -1 ? -4
  serie.special.humide <- ToRaster(CNSW.sub[select.sol,], AE)
  #select.sol <- which(CNSW.sub$SER_SPEC %in% c("J", "H", "J-H" )) #sur la cl?: de 0 - 5 (sec donc)
  #serie.special.sec <- ToRaster(CNSW.sub[select.sol,], AE)
  serie.special.source <- ToRaster(CNSW.sub[which(CNSW.sub$SER_SPEC %in% c("B", "Bo")),], AE)
  serie.special.ravin <- ToRaster(CNSW.sub[which(CNSW.sub$SER_SPEC=="R"),], AE)
  serie.special.affleurementRocheux <- ToRaster(CNSW.sub[which(CNSW.sub$SER_SPEC=="J"),], AE)
  serie.special.affleurInterm <- ToRaster(CNSW.sub[which(CNSW.sub$SER_SPEC=="J-H"),], AE)
  serie.special.solForteP <- ToRaster(CNSW.sub[which(CNSW.sub$SER_SPEC=="H"),], AE)
  serie.special.s <- ToRaster(CNSW.sub[which(CNSW.sub$SER_SPEC=="S"),], AE)
  
  
  # 2019 ; autre solution pour B (-3), R (-2), F (-1) et P (prof 2) 
  
  
  # Phase de profondeur !!! bug, phase 1 ou phase 2!! A corriger et valider.. bof en fait c'est pas un bug, faut juste transvaser la valeur de phase2 dans la colonne phase 1
  # phase_2 est utilis? pour les sols limoneux tr?s caillouteux (charge en ?l?ments grossiers > 50% en volume) - seul les levels "3" et "5" existent
  #CNSW.sub$PHASE_2[is.na(CNSW.sub$PHASE_1) & CNSW.sub$PHASE_2=="3"] <- "3" # voir livret, signification phase 2 semble d?pandant de la (quantit? de) charge ?l?mnt grossiers
  
  levels(CNSW.sub$PHASE_1) <- cbind(levels(CNSW.sub$PHASE_1),"3", "5")
  CNSW.sub$PHASE_1[is.na(CNSW.sub$PHASE_1)==1 & is.na(CNSW.sub$PHASE_2)!=1] <-   CNSW.sub$PHASE_2[is.na(CNSW.sub$PHASE_1)==1 & is.na(CNSW.sub$PHASE_2)!=1]
  
  # ci-dessous: ce n'est pas le bon raisonnement
  #CNSW.sub$PHASE_1[is.na(CNSW.sub$PHASE_1) & CNSW.sub$PHASE_2=="3"] <-  "2"
  #CNSW.sub$PHASE_1[is.na(CNSW.sub$PHASE_1) & CNSW.sub$PHASE_2=="5"] <-  "3"
  
  select.sol <- which(CNSW.sub$PHASE_1 %in% c(ph.dico[ph.dico[,2] %in% c("0", "1"),1],NA))   # attention, j'ajoute les NA ici, ? confirmer
  sol.profond <- ToRaster(CNSW.sub[select.sol,], AE)
  sol.pas.profond <- reclassify(sol.profond, c(0,0.5,1, 0.5,1.5,0),include.lowest=T)
  select.sol <- which(CNSW.sub$PHASE_1 %in% ph.dico[ph.dico[,2] %in% c("2"),1] | CNSW.sub$SER_SPEC=="P")
  prof.2 <- ToRaster(CNSW.sub[select.sol,], AE)
  select.sol <- which(CNSW.sub$PHASE_1 %in% ph.dico[ph.dico[,2] %in% c("3"),1])
  prof.3 <- ToRaster(CNSW.sub[select.sol,], AE)
  select.sol <- which(CNSW.sub$PHASE_1 %in% ph.dico[ph.dico[,2] %in% c("4", "5"),1])
  prof.45 <- ToRaster(CNSW.sub[select.sol,], AE)
  select.sol <- which(CNSW.sub$PHASE_1 %in% ph.dico[ph.dico[,2] %in% c("6"),1])
  prof.6 <- ToRaster(CNSW.sub[select.sol,], AE)
  
  # Drainage et complexe de drainage
  select.sol <- which(CNSW.sub$DRAINAGE %in% c("g", "G"))
  drainage.g <- ToRaster(CNSW.sub[select.sol,], AE)
  select.sol <- which(CNSW.sub$DRAINAGE %in% c("f", "F"))
  drainage.f <- ToRaster(CNSW.sub[select.sol,], AE)
  select.sol <- which(CNSW.sub$DRAINAGE %in% c("e", "E"))
  drainage.e <- ToRaster(CNSW.sub[select.sol,], AE)
  select.sol <- which(CNSW.sub$DRAINAGE %in% c("i", "I"))
  drainage.i <- ToRaster(CNSW.sub[select.sol,], AE)
  select.sol <- which(CNSW.sub$DRAINAGE %in% c("h", "H"))
  # corrigé le complexe A en 2020
  drainage.h <- ToRaster(CNSW.sub[select.sol,], AE)
  select.sol <- which(CNSW.sub$DRAINAGE %in% c("d", "A","D"))
  drainage.d <- ToRaster(CNSW.sub[select.sol,], AE)
  select.sol <- which(CNSW.sub$DRAINAGE %in% c("c", "C"))
  drainage.c <- ToRaster(CNSW.sub[select.sol,], AE)
  select.sol <- which(CNSW.sub$DRAINAGE %in% c("b", "B"))
  drainage.b <- ToRaster(CNSW.sub[select.sol,], AE)
  select.sol <- which(CNSW.sub$DRAINAGE %in% c("a"))
  drainage.a <- ToRaster(CNSW.sub[select.sol,], AE)
  
  # Texture  - voir dicos_cnsw.xls
  select.sol <- which(CNSW.sub$MAT_TEXT %in% dico[dico[,2] %in% c("Z", "S", "P"),1])
  text.ZSP <- ToRaster(CNSW.sub[select.sol,], AE)
  select.sol <- which(CNSW.sub$MAT_TEXT %in% dico[dico[,2] %in% c("L", "A", "E", "U"),1])
  text.LAEU <- ToRaster(CNSW.sub[select.sol,], AE)
  select.sol <- which(CNSW.sub$MAT_TEXT %in% dico[dico[,2] %in% c("G"),1])
  text.G <- ToRaster(CNSW.sub[select.sol,], AE)
  select.sol <- which(CNSW.sub$MAT_TEXT %in% dico[dico[,2] %in% c("Z", "S", "P"),1] & (CNSW.sub$DEV_PROFIL=="a" | CNSW.sub$SUBSTRAT %in% c("ju", "iu", "ku", "n", "nu")) )
  ZSP.enrichi.argiles <- ToRaster(CNSW.sub[select.sol,], AE)
  
  # application de la cl? hydrique
  NH <- AE
  #AE.avec.perte <- AE
  # sans perte
  #AE <- reclassify(AE, c(0,1.5,1, 1.5,2.5,2, 2.5,3.5,3,   3.5,4.5,1),include.lowest=T) 
  NH[] <- 0
  
  NH[serie.special.humide[]==1]<- 0

  NH[apport.nappe[]==1]<- 6 #-4
  
  NH[pas.apport.nappe[]==1 & drainage.g[]==1]<- 6 #-4
  NH[pas.apport.nappe[]==1 & drainage.f[]==1]<- 7 #-3
  NH[pas.apport.nappe[]==1 & drainage.e[]==1]<- 8 #-2
  
  NH[pas.apport.nappe[]==1 & drainage.i[]==1 & AE[]==1]<- 19 #-3RHA
  NH[pas.apport.nappe[]==1 & drainage.i[]==1 & AE[]!=1]<- 7 #-3
  
  NH[pas.apport.nappe[]==1 & drainage.h[]==1 & AE[]==1]<- 18 #-2RHA
  NH[pas.apport.nappe[]==1 & drainage.h[]==1 & AE[]!=1]<- 8 #-2
  
  NH[pas.apport.nappe[]==1 & drainage.d[]==1 & sol.profond[]==1 & AE[]==1]<- 17 #-1RHA
  NH[pas.apport.nappe[]==1 & drainage.d[]==1 & sol.profond[]==1 & AE[]!=1]<- 9 #-1
  
  NH[pas.apport.nappe[]==1 & drainage.d[]==1 & sol.pas.profond[]==1 & AE[]==1]<- 17 #-1RHA
  NH[pas.apport.nappe[]==1 & drainage.d[]==1 & sol.pas.profond[]==1 & AE[]==2]<- 11 #+1
  NH[pas.apport.nappe[]==1 & drainage.d[]==1 & sol.pas.profond[]==1 & AE[]==3]<- 9 #-1
  
  NH[pas.apport.nappe[]==1 & drainage.c[]==1 & (text.LAEU[]==1 | text.G[]==1) & sol.profond[]==1]<- 10 #0
  NH[pas.apport.nappe[]==1 & drainage.c[]==1 & (text.LAEU[]==1 | text.G[]==1) & sol.pas.profond[]==1 & AE[]!=3] <- 11 #1
  NH[pas.apport.nappe[]==1 & drainage.c[]==1 & (text.LAEU[]==1 | text.G[]==1) & sol.pas.profond[]==1 & AE[]==3] <- 10
  
  NH[pas.apport.nappe[]==1 & drainage.c[]==1 & text.ZSP[]==1 & sol.profond[]==1 & AE[]==1]<- 11
  NH[pas.apport.nappe[]==1 & drainage.c[]==1 & text.ZSP[]==1 & sol.profond[]==1 & AE[]!=1]<- 10
  NH[pas.apport.nappe[]==1 & drainage.c[]==1 & text.ZSP[]==1 & sol.pas.profond[]==1 & AE[]==1]<- 12
  NH[pas.apport.nappe[]==1 & drainage.c[]==1 & text.ZSP[]==1 & sol.pas.profond[]==1 & AE[]==2]<- 11
  NH[pas.apport.nappe[]==1 & drainage.c[]==1 & text.ZSP[]==1 & sol.pas.profond[]==1 & AE[]==3]<- 10
  
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.LAEU[]==1 & sol.profond[]==1 & AE[]==1]<- 11
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.LAEU[]==1 & sol.profond[]==1 & AE[]!=1]<- 10
 
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.LAEU[]==1 & prof.2[]==1 & SS[]==1 & AE[]==1]<- 12
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.LAEU[]==1 & prof.2[]==1 & SS[]==1 & AE[]==2]<- 11
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.LAEU[]==1 & prof.2[]==1 & SS[]==1 & AE[]==3]<- 10

  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.LAEU[]==1 & prof.2[]==1 & SS[]==2 & AE[]==1]<- 13
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.LAEU[]==1 & prof.2[]==1 & SS[]==2 & AE[]==2]<- 12
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.LAEU[]==1 & prof.2[]==1 & SS[]==2 & AE[]==3]<- 11#  

  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.LAEU[]==1 & prof.3[]==1 & SS[]==1 & AE[]==1]<- 14
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.LAEU[]==1 & prof.3[]==1 & SS[]==1 & AE[]==2]<- 13
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.LAEU[]==1 & prof.3[]==1 & SS[]==1 & AE[]==3]<- 11
  
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.LAEU[]==1 & prof.3[]==1 & SS[]==2 & AE[]==1]<- 15
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.LAEU[]==1 & prof.3[]==1 & SS[]==2 & AE[]==2]<- 14
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.LAEU[]==1 & prof.3[]==1 & SS[]==2 & AE[]==3]<- 13#   
  
  # drainage b, mat text G
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & sol.profond[]==1 & AE[]==1]<- 11
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & sol.profond[]==1 & AE[]==2]<- 10 # avant il y avait une faute de frappe ici, c'est 0 et pas 1
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & sol.profond[]==1 & AE[]==3]<- 10
  
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & prof.2[]==1 & SS[]==1 & AE[]==1]<- 12
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & prof.2[]==1 & SS[]==1 & AE[]==2]<- 11 # avant il y avait une faute de frappe ici, c'est 1 et pas 2
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & prof.2[]==1 & SS[]==1 & AE[]==3]<- 10
  
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & prof.2[]==1 & SS[]==2 & AE[]==1]<- 13
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & prof.2[]==1 & SS[]==2 & AE[]==2]<- 12
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & prof.2[]==1 & SS[]==2 & AE[]==3]<- 11# vu que les secteurs chaud descendent jusqu'en fond de vall?e, il y a ds occurence de ce type. ! j'ai modifi? la carte des SS, ne descend plus si bas en fond de vall?e
  
  
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & prof.3[]==1 & SS[]==1 & AE[]==1]<- 13
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & prof.3[]==1 & SS[]==1 & AE[]==2]<- 12
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & prof.3[]==1 & SS[]==1 & AE[]==3]<- 11
  
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & prof.3[]==1 & SS[]==2 & AE[]==1]<- 14
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & prof.3[]==1 & SS[]==2 & AE[]==2]<- 13
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & prof.3[]==1 & SS[]==2 & AE[]==3]<- 12#
  
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & prof.45[]==1 & SS[]==1 & AE[]==1]<- 14
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & prof.45[]==1 & SS[]==1 & AE[]==2]<- 13
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & prof.45[]==1 & SS[]==1 & AE[]==3]<- 12

  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & prof.45[]==1 & SS[]==2 & AE[]==1]<- 15
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & prof.45[]==1 & SS[]==2 & AE[]==2]<- 14
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & prof.45[]==1 & SS[]==2 & AE[]==3]<- 13#
  
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & prof.6[]==1  & AE[]!=3]<- 15
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & prof.6[]==1  & AE[]==3]<- 14#
  
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.ZSP[]==1 & sol.profond[]==1 & ZSP.enrichi.argiles[]==1 & AE[]==1]<- 11
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.ZSP[]==1 & sol.profond[]==1 & ZSP.enrichi.argiles[]==1 & AE[]!=1]<- 10
  
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.ZSP[]==1 & sol.profond[]==1 & ZSP.enrichi.argiles[]!=1 & AE[]==1]<- 12
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.ZSP[]==1 & sol.profond[]==1 & ZSP.enrichi.argiles[]!=1 & AE[]==2]<- 11
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.ZSP[]==1 & sol.profond[]==1 & ZSP.enrichi.argiles[]!=1 & AE[]==3]<- 10
  
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.ZSP[]==1 & prof.2[]==1 & SS[]==1 & AE[]==1]<- 13
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.ZSP[]==1 & prof.2[]==1 & SS[]==1 & AE[]==2]<- 12
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.ZSP[]==1 & prof.2[]==1 & SS[]==1 & AE[]==3]<- 11
  
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.ZSP[]==1 & prof.2[]==1 & SS[]==2 & AE[]==1]<- 14
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.ZSP[]==1 & prof.2[]==1 & SS[]==2 & AE[]==2]<- 13
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.ZSP[]==1 & prof.2[]==1 & SS[]==2 & AE[]==3]<- 12#
  
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.ZSP[]==1 & prof.3[]==1 & SS[]==1 & AE[]==1]<- 15
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.ZSP[]==1 & prof.3[]==1 & SS[]==1 & AE[]==2]<- 14
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.ZSP[]==1 & prof.3[]==1 & SS[]==1 & AE[]==3]<- 13#
  
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.ZSP[]==1 & prof.3[]==1 & SS[]==2 & AE[]!=3]<- 15
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.ZSP[]==1 & prof.3[]==1 & SS[]==2 & AE[]==3]<- 14#
  
  NH[pas.apport.nappe[]==1 & drainage.a[]==1]<- 15
  
  # s?rie sp?ciale humide
  NH[serie.special.source[]==1] <- 7 # -3 source
  NH[serie.special.ravin[]==1] <-8 # -2 ravin
  NH[serie.special.s[]==1] <- 9 # -1 fond vallon limoneux
  
  # s?rie sp?ciale seche
  NH[serie.special.affleurementRocheux[]==1 & SS[]==1] <- 13 # froid
  NH[serie.special.affleurementRocheux[]==1 & SS[]==2] <- 15 # chaud : moins de sol qu'en SS froid du ? ?rosion
  NH[serie.special.affleurInterm []==1 & SS[]==1] <- 13 # froid
  NH[serie.special.affleurInterm []==1 & SS[]==2] <- 14 # chaud 
  NH[serie.special.solForteP []==1 & SS[]==1] <- 12 # froid
  NH[serie.special.solForteP []==1 & SS[]==2] <- 14 # chaud : 
  
  #AE <- AE.avec.perte 
  
  if (filename==""){
    cat(paste("total time : " ,round(proc.time()[3] - ptm,0),"\n"))
    return(NH)
  } else {
    writeRaster(NH, filename=filename, format="GTiff",overwrite=T)
    cat(paste("fin carto des niveaux hydriques \n",sep=""))
  }
  #if (fileAE!=""){
  #  writeRaster(AE, filename=fileAE, format="GTiff",overwrite=T)
  #}
  #if (fileSS!=""){
  #  writeRaster(SS.originel, filename=fileSS, format="GTiff",overwrite=T)
  #}
  
  if (0){
  # test: ajout de zone de perte d'eau - manque n?namoins la distinction plateau/versant pour discriminer pr?cis?ment les RHAs
  NH[pas.apport.nappe[]==1 & drainage.d[]==1 & sol.profond[]==1 & AE[]==4]<- 11
  NH[pas.apport.nappe[]==1 & drainage.d[]==1 & sol.pas.profond[]==1 & AE[]==4]<- 12
  NH[pas.apport.nappe[]==1 & drainage.c[]==1 & (text.LAEU[]==1 | text.G[]==1) & sol.profond[]==1 & AE[]==4]<- 11
  NH[pas.apport.nappe[]==1 & drainage.c[]==1 & (text.LAEU[]==1 | text.G[]==1) & sol.pas.profond[]==1 & AE[]==4]<- 12
  NH[pas.apport.nappe[]==1 & drainage.c[]==1 & text.ZSP[]==1 & sol.profond[]==1 & AE[]==4]<- 12
  NH[pas.apport.nappe[]==1 & drainage.c[]==1 & text.ZSP[]==1 & sol.pas.profond[]==1 & AE[]==4]<- 13
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.LAEU[]==1 & sol.profond[]==1 & AE[]==4]<- 12
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.LAEU[]==1 & prof.2[]==1 &  SS[]==1 & AE[]==4]<- 13
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.LAEU[]==1 & prof.2[]==1 &  SS[]==2 & AE[]==4]<- 14
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.LAEU[]==1 & prof.3[]==1 &  SS[]==1 & AE[]==4]<- 14
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.LAEU[]==1 & prof.3[]==1 &  SS[]==2 & AE[]==4]<- 15
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & sol.profond[]==1 & AE[]==4]<- 12
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & prof.2[]==1 & SS[]==1 & AE[]==4]<- 13
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & prof.2[]==1 & SS[]==2 & AE[]==4]<- 14
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & prof.3[]==1 & SS[]==1 & AE[]==4]<- 13
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & prof.3[]==1 & SS[]==2 & AE[]==4]<- 14
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & prof.45[]==1 & SS[]==1 & AE[]==4]<- 14
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & prof.45[]==1 & SS[]==2 & AE[]==4]<- 15
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.G[]==1 & prof.6[]==1 & AE[]==4]<- 15
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.ZSP[]==1 & sol.profond[]==1 &  ZSP.enrichi.argiles[]==1 & AE[]==4]<- 12
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.ZSP[]==1 & sol.profond[]==1 &  ZSP.enrichi.argiles[]!=1 & AE[]==4]<- 13
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.ZSP[]==1 & prof.2[]==1 & SS[]==1 & AE[]==4]<- 14
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.ZSP[]==1 & prof.2[]==1 & SS[]==2 & AE[]==4]<- 15
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.ZSP[]==1 & prof.3[]==1 & SS[]==1 & AE[]==4]<- 15
  NH[pas.apport.nappe[]==1 & drainage.b[]==1 & text.ZSP[]==1 & prof.3[]==1 & SS[]==2 & AE[]==4]<- 15
 
  # le reste ?a doit etre bon avec ce qu'il y a ci-dessus
  if (filename!=""){
    writeRaster(NH, filename=paste(filename,"_avecPerte", sep=""), format="GTiff",overwrite=T)
  }
  }
  
  cat(paste("total time : " ,round((proc.time()[3] - ptm)/60,0)," minutes\n"))
  
}

# retroune A (sans apport/Neutre), B (apport ponctuel) ou C (apport permanent)
# apr?s tergiversation sur le nombre de classes d'apport d'eau, le FEE version 2 en reste ? 3 classes.
# Fran?ois ridremont a g?n?r? une carte pour toutes la RW pour 5 niveaux d'apport d'eau, avec une m?thodo inspir?e de Sevrin Damien 2008 (AC) et de Weiss (FR utilise le tpi ? 75 metre)
# je transpose la m?thodo de Fran?ois en code, avec uniquement 3 classes. Voir doc de Fran?ois donc.
map_ApportEau <- function (shp.polyg.path="/home/lisein/Documents/Lisein_j/AC2016/RelFloristique/Jo/ApportEau_et_NH/shp//zoneTestNeupont.shp"
                             # les 3 sources de donn?es:
                            ,MNT.path = "I://NH_GDL2019//data/MNT_10m_WALLONIA.tif"
                            ,resolution=10   # r?solution de la carte finale, en m
                            ,filename=""
                             #,META=T # save classification details
                            ,DisplayMap=T
                             # vallon encaiss?, bas de versant : 75 m, comme Fran?ois Rid, mais 2 tpi, 1 basic et 1 en pond?ration inv distance.
                            ,rayon.tpi.norm=75
                            ,rayon.pente=20
                           #pour large vall?e plate
                           ,rayon.tpi.inv=200  
                           # petit vallon encaiss?
                           #,rayon.tpi.norm=100 
                           ,seuil.tpi=-12
                           ,META=T
                           ,quietly = F,
                           inondation.path= "I://NH_RW2019//data//alea_inondation_20070626//alea_inondation_20070626.shp"
                           )
                            
{
  ptm <- proc.time()[3]
  if(quietly==F){cat("Chargement des donn?es sources (MNT, CNSW, r?seau hydro, al?a inondation) \n")}
  # note rayon focalweigth: 3 fois le rayon donn?. If type=Gauss the size of sigma, and optionally another number to determine the size of the matrix returned (default is 3 times sigma)
  buf <- 4*max(rayon.tpi.norm,rayon.pente,rayon.tpi.inv) # en th?orie l'effect de bord du focal, c'est 2 fois le rayon, mais en r?alit? c'est un peu plus (car les r?solutions ne sont pas les meme et arrondi de focalweight).
  # pr?paration des couches
  zone <- readShapePoly(shp.polyg.path)
  # Lidar 10 m de r?solution
  MNT <- raster(MNT.path)
 
  #CNSW <- readOGR(dsn=CNSW.path,layer="CNSW_poly")
  # finalement, au dire de Hugues, on aimerai bien se passer un maximum de l'info p?do. On n'avait que ?a avant, avec principalement les sol p et P, pour lesquels on ?tait sur qu'il ?tait en fond de vall?e (apport C). Mais il y a de nombreux fond de vall?e sans sol p, plus les villes (pas de sols)
  # maintenant on peut d?limiter les zones C sur base d'une combinaison r?seau hydro+ topo (mnt.relatif au cour d'eau, cf travail d'adrien, et tpi invers pour rep?rer les grandes plaines alluviales)
  CNSW <- open_cnsw_tuiles(zone=zone)
  cat("CNSW : done")
  #CNSW <- readOGR(dsn=CNSW.path,layer=CNSW.name)
  # reseau hydro -- version du SPW, fabriqu? sur base du lidar, en cour de validation (probl?me sur pont autoroute)
  #hydro <- readShapeLines(hydro.path)
  hydro <- open_hydro_tuiles(zone=zone)
  
  # on clippe les couches et raster qui sont toutes trop lourdes
  MNT.crop <- crop(MNT, gBuffer(zone,width=buf))
  AE.raster <- MNT.crop
  
  sub <- gIntersects(gBuffer(zone,width=buf), hydro, byid = TRUE)
  hydro.sub <- hydro[ which(sub),]
  
  sub <- gIntersects(gBuffer(zone,width=buf), CNSW, byid = TRUE)
  CNSW.sub <<- CNSW[which(sub),]
  
  # al?a d'inondation ; tout en apport permanent
  inondation <- readShapePoly(inondation.path)
  sub <- gIntersects(gBuffer(zone,width=buf), inondation, byid = TRUE)
  inondation.sub <- inondation[ which(sub),]
  
  # plage de sols d'alimentation constante : 
  #sans.dev.prof <- CNSW.sub[which(CNSW.sub$DEV_PROFIL %in% c("p", "P")),] # sols sans profis: apport de sols constamment, donc pas le temps de diff?rencier des horizons. Ces sols sont hyper li?s ? la topographie
  #drainage.insuffisant <- CNSW.sub[which(CNSW.sub$DRAINAGE %in% c("e", "f", "g", "h", "i", "F", "G", "I")),] # drainage inssufisant
  #limono.caillouteux.fv <- CNSW.sub[which(CNSW.sub$MAT_TEXT %in% c("G", "(G)") & is.null(CNSW.sub$CHARGE)),] # sols limono-caillouteux des vall?es et d?pressions (donc SANS nature de charge)
  #phase.allu <- CNSW.sub[which(CNSW.sub$PHASE_7=="(1)"),] # phase alluviale
  
  # voir Mode_operatoire sevrin juin 2008 fourni avec carte apport hydrique + mode ope FR 2016
  #id.apport <- which(CNSW.sub$SER_SPEC %in% c("B", "Do","R", "S","B/o", "Ma") | CNSW.sub$DEV_PROFIL %in% c("p", "P") | CNSW.sub$DRAINAGE %in% c("e", "f", "g", "h", "i", "F", "G", "I") | CNSW.sub$PHASE_7=="(1)" | (CNSW.sub$MAT_TEXT %in% c("G", "(G)") & is.null(CNSW.sub$CHARGE)))
  
  #id.p.bon.drainage <- which(CNSW.sub$DEV_PROFIL %in% c("p", "P") & CNSW.sub$DRAINAGE %in% c("a", "b", "c" ,"d", "A", "B", "D"))
  
  #apport <- CNSW.sub[id.apport,] # sols sans profis: apport de sols constamment, donc pas le temps de diff?rencier des horizons. Ces sols sont hyper li?s ? la topographie
  #apport <- gUnaryUnion(apport)
  
  if(quietly==F){cat(paste("elapsed time : ",round(proc.time()[3] - ptm,0),"\n"))}
  ptm2 <- proc.time()[3]
  
  # al?a d'inondation
  alea.inondation.raster <- ToRaster(inondation.sub, AE.raster)
  
  #writeRaster(alea.inondation.raster, filename="test2", format="GTiff",overwrite=T)
  
  
  # buffer autour du reseau hydro : utile uniquement pour apport variable ?coulement temporaire
  if(quietly==F){cat("Calcul de la zone tampon autour du r?seau hydrique pour d?tection apport permanent \n")}
  my.add <- 10
  temporaire <- hydro.sub$PERMANENCE !=1
  temporaire[is.na(temporaire)] <- TRUE
  buf.width <- rep(0,length(hydro.sub))
  #buf.width[which(hydro.sub$CATEG %in% c("NA") && temporaire)] <- 240 + my.add# navigable
  #buf.width[which(hydro.sub$CATEG %in% c("01", "N1") && temporaire)] <- 120 + my.add
  #buf.width[which(hydro.sub$CATEG %in% c("02", "N2")) && temporaire] <- 60+ my.add
  #buf.width[which(hydro.sub$CATEG %in% c("03", "N3", "NC", "NR")) && temporaire] <- 30+ my.add
  
  buf.width[which(temporaire)] <- 30
  buf.hydro.tempo <- gUnaryUnion(gBuffer(hydro.sub,width=buf.width,byid=T,quadsegs=3))  # pas union, sinon pas de rasterize en parrall?le
  buf.width <- rep(0,length(hydro.sub))
  buf.width[which(!temporaire)] <- 30
  buf.hydro.perm <- gUnaryUnion(gBuffer(hydro.sub,width=buf.width,byid=T,quadsegs=3))
  # pour les utiliser en combinaison avec la topo, conversion en raster
  #buf.hydro.tempo <- gBuffer(hydro.sub,width=buf.width,byid=T)
  if(quietly==F){cat("-")}

  buff.hydro.tempo.raster <- ToRaster(buf.hydro.tempo,AE.raster)
  buff.hydro.perm.raster <- ToRaster(buf.hydro.perm,AE.raster)
  #hydro.raster <- ToRaster(hydro.sub,AE.raster)
  
  #buff.hydro.raster <- rasterize(buf.hydro,AE.raster,field=1,background=0)
  if(quietly==F){cat("-")}
  temp <- gBuffer(hydro.sub,width=xres(AE.raster))
  hydro.raster <- rasterize(temp,AE.raster,field=1,background=0)# je peux gagner un peu de temps ici, plus rapide de rasterizer des polygones que des lignes
  if(quietly==F){cat("-\n")}
  # on classe les sols d'apport en apport permanent sur base de leur proximit? au r?seau hydro
  #apport.permanent<- apport[c(which(gOverlaps(apport, buf.hydro, byid=T)),which(gContains(buf.hydro,apport, byid=T))),]
  
  # FR classait tout les sols "apport" en apport permanent ou non permanent, mais c'est inad?quat car un sol ? mauvais drainage en plateau n'a pas d'apport. 
  #apport.non.permanent <- apport[-c(which(gOverlaps(apport, buf.hydro, byid=T)),which(gContains(buf.hydro,apport, byid=T))),]
 
  # certains sols re?oivent un apport permanent d'une nappe phr?atique (? engorgement d'eau permanent - drainage). Il est ?vident qu'on ne peut pas les distinguer sur base de crit?res topo ou hydro (la nappe n'est pas visible sur les cartes),
  # c'est donc via la p?do qu'on les d?tecte. 
  if(quietly==F){cat(paste("elapsed time : ",round(proc.time()[3] - ptm2,0),"\n"))}
  ptm2 <- proc.time()[3]
  if(quietly==F){cat("D?tection des apport permanent en provenance d'une nappe phr?atique (sur base p?do) \n")}
  id.apport.nappe <- which(CNSW.sub$SER_SPEC %in% c("B", "Do","R", "S","B/o", "Ma") | CNSW.sub$DRAINAGE %in% c("e", "f", "g") | CNSW.sub$MAT_TEXT %in% c("V","W"))
  apport.P.nappe<- CNSW.sub[id.apport.nappe,]
  apport.P.nappe.raster <- ToRaster(apport.P.nappe, AE.raster)

  #------------------------------------------------------
 
  if(quietly==F){cat("Calcul des indices topographiques (TPI locac basic, TPI local pond?r? inverse des distances, TPI large, pente) \n")}
  MNT.crop.resampled <- DeZoom(MNT.crop,rayon.pente,method="bilinear")
  slope.raster <- terrain(MNT.crop.resampled, opt='slope',neighbors=8,unit='degrees')
   # TPI = topographic position index, alti(i)-moyenne alt(i+rayon)
  # je fait un TPI avec alt moyenne invers?ment pond?r?e par la distance, afin de discriminer les zones plates localement mais globalement en fond de vall?e
  # le tpi c'est le plus long en temps de calcul, je dezoom avant sinon trop long:
  
  MNT.crop.resampled <- MNT.crop # full resolution - petite ?chelle
  # attention, la fonction focal mal utilis?e produit un effet de bord consid?rable. voir lien ci-dessous, mais non applicable car je suis avec une pond?ration (sum), et en plus inverse des distances outch, donc je fait un buffer.
  # http://gis.stackexchange.com/questions/187410/how-do-i-get-rid-of-edge-effects-while-using-focal-in-r-to-smooth-a-raster
  #alt.mean <- mean(MNT.crop.resampled[],na.rm=T)
  pond.TPI.norm <- focalWeight(MNT.crop.resampled, d=rayon.tpi.norm, "circle")
  TPI.basic.raster <- MNT.crop.resampled-focal(MNT.crop.resampled, w=pond.TPI.norm, fun="sum", na.rm=T,pad=F)
  pond.TPI.norm <- focalWeight(MNT.crop.resampled, d=rayon.tpi.norm/2, "circle")
  TPI.basic2.raster <- MNT.crop.resampled-focal(MNT.crop.resampled, w=pond.TPI.norm, fun="sum", na.rm=T,pad=F)
  
  MNT.crop.resampled <- DeZoom(MNT.crop,2*resolution) # on descend la r?solution, car tpi ? large ?chelle
  # tpi inv grand rayon
  pond.TPI <- focalWeight(MNT.crop.resampled, d=rayon.tpi.inv, "Gauss")
  # enlever les valeurs les plus basses (coins)
  pond.TPI[pond.TPI<quantile(pond.TPI,0.25)]<-quantile(pond.TPI,0.35)
  # inverser la pond?ration
  pond.TPI.inv <- 1/pond.TPI 
  pond.TPI.inv <- pond.TPI.inv /sum(pond.TPI.inv) 
  TPI.inv.raster <- MNT.crop.resampled-focal(MNT.crop.resampled, w=pond.TPI.inv, fun="sum", na.rm=T,pad=F)
  # je part d'un filtre gaussien que je modifie
  # tpi petit rayon, mais en pond?ration inverse ?galement - test
  pond.TPI <- focalWeight(MNT.crop.resampled, d=rayon.tpi.norm, "Gauss")
  # enlever les valeurs les plus basses (coins)
  pond.TPI[pond.TPI<quantile(pond.TPI,0.25)]<-quantile(pond.TPI,0.35)
  # inverser la pond?ration
  pond.TPI.inv <- 1/pond.TPI 
  pond.TPI.inv <- pond.TPI.inv /sum(pond.TPI.inv) 
  TPI.norm.raster <- MNT.crop.resampled-focal(MNT.crop.resampled, w=pond.TPI.inv, fun=sum,na.rm=T,pad=F)
  
  if(quietly==F){cat(paste("elapsed time : ",round(proc.time()[3] - ptm2,0),"\n"))}
  ptm2 <- proc.time()[3]
  if(quietly==F){cat("D?tection des zones d'apport quasi permanent (r?seau hydro + topographie) \n")}
  MNT.hydro <- MNT.crop
  MNT.hydro[hydro.raster[]==0] <- NA
  # it?ration de calcul de moyenne avec focale faire grandir la r?gion autour du r?seau hydro
  window <- focalWeight(MNT.hydro, d=2*xres(MNT.hydro), "circle")
  #window[window[]>0]<-1
  window[]<-1
  for (i in 1:20){
    test <- focal(MNT.hydro, w=window, fun=mean, na.rm=T,pad=F)
    MNT.hydro[is.na(MNT.hydro[])==T & is.na(test[])==F] <- test[is.na(MNT.hydro[])==T & is.na(test[])==F] 
    cat("--")
  }
  # c'est donc un mnt relatif ? la hauteur du niveau de l'eau, valable localement autour des ruisseaux et axes hydriques
  mnt.rel <- MNT.crop - MNT.hydro 
  # attention, masquer avec les deux buff Hydro (permanent et variable) NON necessaire, j'emploi les deux conjointenemen par apr?s
  #mnt.rel <- mask(mnt.rel,buff.hydro.raster,maskvalue=0)
  cat("\n")
  
  if(quietly==F){cat(paste("elapsed time : ",round(proc.time()[3] - ptm2,0),"\n"))}
  ptm2 <- proc.time()[3]
  
  # toutes les couches doivent avoir la m?me r?solution et le meme extend (attention, je n'ai pas mis le bilinear, plus rapide mais moins pr?cis)
  cat("pr?paration des raster\n")
  TPI.basic.raster <-resample(TPI.basic.raster,AE.raster)
  TPI.basic2.raster <-resample(TPI.basic2.raster,AE.raster)
  TPI.norm.raster <- resample(TPI.norm.raster,AE.raster)
  TPI.inv.raster <- resample(TPI.inv.raster,AE.raster)
  slope.raster <- resample(slope.raster,AE.raster)
  
  #apport.P.raster <- rasterize(apport.permanent, AE.raster,field=1,background=0)
  
  if(quietly==F){cat("Cr?ation de la carte d'apport d'eau \n")}
  
  # les seuillages
  class.tpi.basic <- cut(TPI.basic.raster, breaks=c(-Inf,-4,0.5,+Inf))
  class.tpi.basic2 <- cut(TPI.basic2.raster, breaks=c(-Inf,3,5,+Inf)) # pour d?tecter cr?tes (perte en eau) : rayon "petit"
  class.tpi.basic3 <- cut(TPI.basic.raster, breaks=c(-Inf,3,+Inf))  # pour d?tecter cr?tes (perte en eau) : rayon "moyen"
  class.tpi.norm <- cut(TPI.norm.raster, breaks=c(-Inf,seuil.tpi,+Inf)) # vallon encaiss? .  rayon de 75 en gauss,fonctionne bien avec seuil de -5. bon, vu que je le combine maintenant avec le tpi basic, je remonte le seuil ? -2
  class.tpi.inv <- cut(TPI.inv.raster, breaks=c(-Inf,-10,+Inf)) 
  class.slope <- cut(slope.raster, breaks=c(0,3,+Inf)) 
  class.mnt.rel <- cut(mnt.rel, breaks=c(-Inf,1.5,+Inf))# je peux me montrer s?v?re car concerne par d?finition les petits cours d'eau - al?a inondation prend le reste 
  
  # arbre de d?cision/classification
  # attention, donner des num?ro de classif qui soient en gradation avec les classes d'apport, sinon lors du resample il moyenne des pixels et change de classe de mani?re anormale
  
  AE.raster[] <- 0 # met les valeurs ? z?ro
  
  # apport quati constant al?a inondation
  AE.raster[alea.inondation.raster==1] <- 1
  AE.raster[AE.raster[]==0 & mnt.rel[]<2 & buff.hydro.perm.raster[]==1] <- 1
  AE.raster[AE.raster[]==0 & mnt.rel[]<3 & buff.hydro.perm.raster[]==1] <- 2
  
  AE.raster[AE.raster[]==0 & mnt.rel[]<3 & buff.hydro.tempo.raster[]==1] <- 2 # apport variable car ?coulement variable
  
  # plus n?cessaire, al?a inondation l'a fait
  # zone de fond de vall?e large en zone peu pentue (pas toujour enti?rement d?tect? autour du r?seau hydro, car grande surface)
  AE.raster[class.tpi.inv[]==1 & class.slope[]==1 & AE.raster[]==0] <- 3 # apport quasi constant # non variable!
  
  # apport permanent depuis une nappe phr?atique (sur base topo, l? ou la p?do n'est plus capable de voir sous le sol)
  AE.raster[apport.P.nappe.raster[]==1 & AE.raster[]==0] <- 3
 
  # apport lat?raux non permanent, uniquement sur base topo
  AE.raster[TPI.norm.raster[]<(-10) & TPI.basic.raster[]<(-5) & AE.raster[]==0] <- 4
  AE.raster[TPI.norm.raster[]<(-15) & TPI.basic.raster[]<(-2) & AE.raster[]==0] <- 5
  #AE.raster[class.tpi.basic[]==1 & AE.raster[]==0] <- 5
  
  # sans apport : le reste (la majorit? en fait)
  AE.raster[AE.raster[]==0] <- 6
  
  # perte en eau - class tpi basic, rayon moyen, tpi basic 2, rayon plus restreint
  AE.raster[class.tpi.basic2[]==3] <- 7 # tr?s sur?lev? localement
  AE.raster[class.tpi.basic3[]==2 & class.tpi.basic2[]==2] <- 7 # moins sur?lev? localement mais tout de m?me sur?lev? globalement
  
  # enlever le bord, pour ?viter effet de bord
  AE.raster.crop <- crop(AE.raster,zone)
  
  # de changement de r?soltion s'effectue ? la fin, sinon perte de qualit?. la carte des m?tadonn?e est sensible ? un d?zoom, je le fait en majority 
  AE.raster.resampled <- DeZoom(AE.raster.crop,resolution,my.fun=modal)
  
  AE.raster.resampled <- reclassify(AE.raster.resampled, c(0,0.5,0, 0.5,1.5,1   ,1.5,2.5,2  ,2.5,3.5,3, 3.5,4.5,4, 4.5,5.5,5, 5.5,6.5,6,  6.5,7.5,7))
  
  if(META==T & filename!=""){
    writeRaster(AE.raster.resampled, filename=paste(filename,"META_CLASSIF",sep="_"), format="GTiff",overwrite=T)
  }
  
  # attribuer les sous-secteurs
  AE.raster.resampled.reclass <- reclassify(AE.raster.resampled, c( 1,1,3, 2,2,2,  3,3,2  ,4,4,2  ,5,5,2, 6,6,1, 7,7,4),include.lowest=T, right=NA)
  
  if (DisplayMap==T){
    plot(AE.raster.resampled.reclass,col=c("blue","green", "white", "yellow"), main="Carte des apports d'eau")
  }
  
  # fin fonction
  if (filename==""){
    if(quietly==F){cat(paste("total time : " ,round(proc.time()[3] - ptm,0),"\n"))}
    return(AE.raster.resampled.reclass)
  } else {
    writeRaster(AE.raster.resampled.reclass, filename=filename, format="GTiff",overwrite=T)
    if(quietly==F){cat(paste("fin carto des apport d'eau (Sevrin Damien 2008 + Fran?ois Ridremont 2016 + Jo lisein 2017 et 2019). \nVoir le Fichier ", filename,".tif qui contient les r?sultats. 1=sans apport, 2= apport non permanent, 3=apport permanent \n ? comparer avec carte app eau de sevrin \n",sep=""))}
  }
  
  
  if(quietly==F){cat(paste("total time : " ,round(proc.time()[3] - ptm,0),"\n"))}
       
  # purge - pas n?cessaire en fait car c'est une fonction, donc purge automatique de son environnement. Plut?t utile dans des boucles.
  rm(buf,MNT,MNT.crop,MNT.crop.resampled,TPI.norm.raster,TPI.basic.raster,TPI.inv.raster,AE.raster,AE.raster.crop,AE.raster.resampled,apport.P.nappe,CNSW,buff.hydro.tempo.raster,buff.hydro.perm.raster,buf.hydro,class.mnt.rel,class.slope,class.tpi.inv,class.tpi.norm,class.tpi.basic,hydro,hydro.raster,apport.P.nappe.raster,MNT.hydro,mnt.rel,slope.raster,alea.inondation.raster)
}



map_SousSecteur <- function (shp.polyg.path="D://Lisein_j//AC2016//RelFloristique//Jo//zoneTestNeupont.shp"
                                 ,MNT.path = "data/MNT_10m_WALLONIA.tif"
                                 ,resolution=20   # r?solution de la carte finale, en m, doit ?tre un multiple de la r?solution du MNT
                                 ,filename=""
                                 ,META=T # save classification details
                                 ,DisplayMap=T
                                  # rayon pour les indices calcul?s
                                 # la pente intervient d?s le premier choix dans la cl?, un grand rayon va lisser la zone, mais il y a dans tt les cas une compensation pente/hillshade pour la discrimination des secteurs froid en pente moyenne.
                                  # c'est mieux de garder un grand rayon de pente, qui va gommer les petit vallon de 30 m de largeur de versant en secteur chaud (trop petit que pour etre consid?r?) 
                                 ,rayon.pente=80
                                  ,rayon.exposition=50
                                  #pour large vall?e plate
                                 ,rayon.tpi.inv=200  
                                  # petit vallon encaiss?
                                  ,rayon.tpi.norm=75  
                                  # n'as qu'une relation indirecte avec l'exposition, pour le hillshade on a besoin de pente et aspect.
                                 ,rayon.hillshade=30
                                  , quietly=F)
{
  if(quietly==F){cat("Calcul des sous-secteurs \n")}
  # pr?paration des couches
  zone <- readShapePoly(shp.polyg.path)
  buf <- 4*max(rayon.tpi.norm,rayon.pente,rayon.tpi.inv) # en th?orie l'effect de bord du focal, c'est 2 fois le rayon, mais en r?alit? c'est un peu plus.
  # Lidar 10 m de r?solution
  MNT <- raster(MNT.path)
  # avec un buffer car effet de bord (pente, aspect, tpi, etc)
  MNT.crop <- crop(MNT, gBuffer(zone,width=buf))
  # lissage du MNT, pour ?viter de diff?rencier des SS dans un vallon de 40 m de large, et pour une comparaison plus ais?e avec 1) carte FR 2013 et 
  #2) carte Delvaux et Gal (digitalis? par FR et fred henrotay - malheureusement ils distinguent fond de vall?e ? part des ss)
  fenetre.filtre <- focalWeight(MNT, d=10, "Gauss")
  MNT.crop <- focal(MNT.crop, w=fenetre.filtre, fun=sum, na.rm=F)
  
  
  # calcul des indices topo utilis?e
  # changer la r?solution pour coh?rence entre la fonction terrain() qui fonctionne avec 4.5 pixels de rayon, et le diametre renseign? comme argument de la fonction
  # modif 2019 avec Lucie, j'avais mal interpréter le fonctionnement des arguments de terrain slope. comment ci-dessus c'est de la merde.
  # slope
  #MNT.crop.resampled <- DeZoom(MNT.crop,rayon.pente/4.5,method="bilinear")
  slope.raster <- terrain(MNT.crop, opt='slope', neighbors=8, unit='degrees')
  # aspect - exposition
  #MNT.crop.resampled <- DeZoom(MNT.crop,rayon.exposition/4.5)
  aspect.raster <- terrain(MNT.crop, opt='aspect',neighbors=4,unit='degrees')
  # TPI = topographic position index, alti(i)-moyenne alt(i+rayon)
  # je fait un TPI avec alt moyenne invers?ment pond?r?e par la distance, afin de discriminer les zones plates localement mais globalement en fond de vall?e
  # le tpi c'est le plus long en temps de calcul, je dezoom avant sinon trop long:
  MNT.crop.resampled <- DeZoom(MNT.crop,2*resolution)
  
  pond.TPI.norm <- focalWeight(MNT.crop.resampled, d=rayon.tpi.norm, "circle")
  TPI.basic.raster <- MNT.crop.resampled-focal(MNT.crop.resampled, w=pond.TPI.norm, fun="sum", na.rm=T,pad=F)
  
  pond.TPI.norm <- focalWeight(MNT.crop.resampled, d=rayon.tpi.norm, "Gauss")
  TPI.norm.raster <- MNT.crop.resampled-focal(MNT.crop.resampled, w=pond.TPI.norm, fun=sum, na.rm=T,pad=F)
  # je part d'un filtre gaussien que je modifie
  pond.TPI <- focalWeight(MNT.crop.resampled, d=rayon.tpi.inv, "Gauss")
  # enlever les valeurs les plus basses (coins)
  pond.TPI[pond.TPI<quantile(pond.TPI,0.25)]<-quantile(pond.TPI,0.35)
  # inverser la pond?ration
  pond.TPI.inv <- 1/pond.TPI 
  pond.TPI.inv <- pond.TPI.inv /sum(pond.TPI.inv) 
  TPI.inv.raster <- MNT.crop.resampled-focal(MNT.crop.resampled, w=pond.TPI.inv, fun=sum, na.rm=T,pad=F)
  
  # calcul hillshade
  #MNT.crop.resampled <- DeZoom(MNT.crop,rayon.hillshade/4.5,method="bilinear")
  slope.raster.rad <- terrain(MNT.crop, opt='slope',neighbors=8,unit='radians')
  aspect.raster.rad <-terrain(MNT.crop, opt='aspect',neighbors=8,unit='radians')
  # hillshade modif 2019pour moyenne pendant période de végétation
  hill.ete <- hillShade(slope.raster.rad, aspect.raster.rad, 
                        angle=63.12, direction=180, normalize=T)
  
  # Hillshade au solstice d'hiver sous 180? d'angle solaire et 16.25? de hauteur solaire [0-255]
  #hill.hiver <- hillShade(slope.raster.rad, aspect.raster.rad, angle=16.25, direction=180, normalize=T)
  
  # Hillshade ? l'?quinoxe 20 mars sous 180 d'angle solaire et 39.52 de hauteur solaire [0-255]
  hill.mars <- hillShade(slope.raster.rad, aspect.raster.rad, 
                         angle=39.52, direction=180, normalize=T)
  
  # Hillshade ? l'?quinoxe 23 sept sous 180 d'angle solaire et 39.62 de hauteur solaire [0-255]
  hill.sept <- hillShade(slope.raster.rad, aspect.raster.rad, 
                         angle=39.62, direction=180, normalize=T)
  
  hillshade.raster <- mean(hill.mars ,hill.ete, hill.sept)

  # creation du raster de sortie
  SS.raster <- MNT.crop
  SS.raster[] <- 0 # met les valeurs ? z?ro
  
  # toutes les couches doivent avoir la m?me r?solution (attention, je n'ai pas mis le bilinear, plus rapide mais moins pr?cis)
  slope.raster <- resample(slope.raster,SS.raster)
  aspect.raster <- resample(aspect.raster,SS.raster)
  hillshade.raster <- resample(hillshade.raster,SS.raster)
  TPI.norm.raster <- resample(TPI.norm.raster,SS.raster)
  TPI.inv.raster <- resample(TPI.inv.raster,SS.raster)
  TPI.basic.raster <- resample(TPI.basic.raster,SS.raster)
 
  # cl? des sous-secteur selon Delvaux et Galoux [1962]
  
  # DetG : seuil entre 15 et 20 %, soit entre 8.5 et 11.3 --> grosse fourchette. on garde 8.5 com seuil, c'est d?j? bien pentu.
  class.slope <- cut(slope.raster, breaks=c(0,8.5,100)) #1 = peu de pente, 2= forte pente
  # cours de Hugues C (fait par FR): seuil de 292.5 et 112.5. DetG: 285 et 125. Je fait un buffer autour de ces limites, sous secteur m?sotherme mis en neutre/principal
  buff.aspect <- 12.5 # propos? par FR
  class.aspect <- reclassify(aspect.raster, c(125+buff.aspect,285-buff.aspect,1  ,125-buff.aspect,125+buff.aspect,3 ,285-buff.aspect,285+buff.aspect,3 ,0,125-buff.aspect,2, 285+buff.aspect,360,2)) #1 = chaud, 2= froid , 3 principal/neutre/m?sotherme
  class.hillshade <- cut(hillshade.raster,breaks=c(0,130,255)) # 1= zone ombrag?e
  class.tpi.inv <- cut(TPI.inv.raster, breaks=c(-Inf,-15,+Inf)) #1 = en vallon, 2=  position surr?lev? .. -30: les larges vallons dans une plaine, avec seulement 30 m de d?nivell? avec leur entourage, sont pas en secteur froid (lesse dans famenne et calestienne)
  class.tpi.norm <- cut(TPI.norm.raster, breaks=c(-Inf,-10,+Inf)) # vallon encaiss? 
  # ajout
  class.tpi.basic <- cut(TPI.basic.raster, breaks=c(-Inf,-5,-1,+Inf))

  # arbre de d?cision/classification
 
  # zone pentue
  SS.raster[class.slope[]==2 & class.aspect[]==1] <- 1 #SS chaud
  SS.raster[class.slope[]==2 & class.aspect[]==2] <- 2 #SS froid
  SS.raster[class.slope[]==2 & class.aspect[]==3] <- 3 #SS neutre/meso/principal
  
  # zone peu ?clair?e : partie inf?rieure toujour ombrag? des grands versants
  SS.raster[(class.slope[]==1 | class.aspect[]==3) & class.hillshade[]==1] <- 4 #SS froid
  
  # zone de fond de vall?e large en zone peu pentue
  SS.raster[class.slope[]==1 & class.tpi.inv[]==1 & slope.raster[]<3] <- 5 #SS froid
  
  # zone de fond de vall?e ?troite mais peu pentue
  SS.raster[class.slope[]==1 & class.tpi.norm[]==1 & class.tpi.inv[]!=1] <- 6 #SS froid
  # zone de fond de vall?e ?troite et pentue 
  SS.raster[((class.slope[]==1 & slope.raster[]>3)|class.aspect[]==3) & class.tpi.norm[]==1 ] <- 6 #SS froid
  
  # ajout 08 2016 : les fond de vallon encaiss? sont en exposition jusqu'en bas, c'est mal, cf bois de lairi
  SS.raster[class.tpi.norm[]==1 & class.tpi.basic[]!=3 & SS.raster[]==0] <- 7
  SS.raster[class.tpi.basic[]==1 & SS.raster[]==0] <- 7
  # compensation exposition par encaissement
  SS.raster[class.tpi.norm[]==1 & class.tpi.basic[]!=3  & SS.raster[]==1] <- 7
  SS.raster[class.tpi.basic[]==1 & SS.raster[]==1] <- 7
  
  # sous secteur neutre: le reste
  SS.raster[SS.raster[]==0] <- 7 #SS neutre
  
  # enlever le bord, pour ?viter effet de bord
  SS.raster <- crop(SS.raster,zone)
  
  if(META==T & filename!=""){
    writeRaster(SS.raster, filename=paste(filename,"META_CLASSIF",sep="_"), format="GTiff",overwrite=T)
  }
  
  # attribuer les sous-secteurs
  SS.raster <- reclassify(SS.raster, c(1,1,3,  2,2,1,  3,3,2  ,4,4,1 ,5,5,1,   6,6,1,  7,7,2 ),include.lowest=T, right=NA)
  
  # de changement de r?soltion s'effectue ? la fin, sinon perte de qualit?
  SS.raster <- DeZoom(SS.raster,resolution)
  SS.raster <- reclassify(SS.raster, c( 0,0.5,0, 0.5,1.5,1   ,1.5,2.5,2  ,2.5,3,3))
  
  if (DisplayMap==T){
     plot(SS.raster,col=c("blue","grey","red"), main="Carte des sous-secteurs")
  }
  
  # fin fonction
  if (filename==""){
    return(SS.raster)
  } else {
    writeRaster(SS.raster, filename=filename, format="GTiff",overwrite=T)
    cat(paste("fin carto des sous secteurs (Delvaux et Galoux, 1962). \nVoir le Fichier ", getwd(),"//", filename,".tif qui contient les r?sultats. 1=ss froid, 2=ss neutre, 3=ss chaud \n",sep=""))
  }
  
  # purge
  rm(MNT,MNT.crop,MNT.crop.resampled,slope.raster,slope.raster.rad,aspect.raster,aspect.raster.rad,TPI.norm.raster,TPI.inv.raster,hillshade.raster,hill.ete,hill.hiver,SS.raster)
  if(quietly==F){cat("Fin calcul des sous-secteurs \n")}
  }


# 2020 pour le FEE, on souhaite prendre en compte les risques li?s ? la situation topographique
map_topo <- function (shp.polyg.path="D://Lisein_j//AC2016//RelFloristique//Jo//zoneTestNeupont.shp"
                             ,MNT.path = "data/MNT_10m_WALLONIA.tif"
                             ,resolution=10   # r?solution de la carte finale, en m, doit ?tre un multiple de la r?solution du MNT
                             ,filename=""
                             ,META=T # save classification details
                             ,DisplayMap=T
                             # rayon pour les indices calcul?s
                             # la pente intervient d?s le premier choix dans la cl?, un grand rayon va lisser la zone, mais il y a dans tt les cas une compensation pente/hillshade pour la discrimination des secteurs froid en pente moyenne.
                             # c'est mieux de garder un grand rayon de pente, qui va gommer les petit vallon de 30 m de largeur de versant en secteur chaud (trop petit que pour etre consid?r?) 
                             ,rayon.pente=80
                             ,rayon.exposition=50
                             #pour large vall?e plate
                             ,rayon.tpi.inv=200  
                             # petit vallon encaiss?
                             ,rayon.tpi.norm=75  
                             # n'as qu'une relation indirecte avec l'exposition, pour le hillshade on a besoin de pente et aspect.
                             ,rayon.hillshade=30
                             , quietly=F
                            , inondation.path= "I://NH_RW2019//data//alea_inondation_20070626//alea_inondation_20070626.shp")
{
  if(quietly==F){cat("Calcul des situations topographiques \n")}
  # pr?paration des couches
  zone <- readShapePoly(shp.polyg.path)
  buf <- 4*max(rayon.tpi.norm,rayon.pente,rayon.tpi.inv) # en th?orie l'effect de bord du focal, c'est 2 fois le rayon, mais en r?alit? c'est un peu plus.
  # Lidar 10 m de r?solution
  MNT <- raster(MNT.path)
  # avec un buffer car effet de bord (pente, aspect, tpi, etc)
  MNT.crop <- crop(MNT, gBuffer(zone,width=buf))
  # lissage du MNT, pour ?viter de diff?rencier des SS dans un vallon de 40 m de large, et pour une comparaison plus ais?e avec 1) carte FR 2013 et 
  #2) carte Delvaux et Gal (digitalis? par FR et fred henrotay - malheureusement ils distinguent fond de vall?e ? part des ss)
  fenetre.filtre <- focalWeight(MNT, d=10, "Gauss")
  MNT.crop <- focal(MNT.crop, w=fenetre.filtre, fun=sum, na.rm=F)
  
  # al?a d'inondation ; tout en fond de vall?. NON!!! car on ne veux pas les plaines alluviales, uniquement les vall?es ?troites qui, elles on des risque d'humidit? (risque sanitaire) et de froid
  #inondation <- readShapePoly(inondation.path)
  #sub <- gIntersects(gBuffer(zone,width=buf), inondation, byid = TRUE)
  #inondation.sub <- inondation[ which(sub),]
  #alea.inondation.raster <- ToRaster(inondation.sub, MNT.crop)
  
  # calcul des indices topo utilis?e
  # changer la r?solution pour coh?rence entre la fonction terrain() qui fonctionne avec 4.5 pixels de rayon, et le diametre renseign? comme argument de la fonction
  # modif 2019 avec Lucie, j'avais mal interpréter le fonctionnement des arguments de terrain slope. comment ci-dessus c'est de la merde.
  # slope
  #MNT.crop.resampled <- DeZoom(MNT.crop,rayon.pente/4.5,method="bilinear")
  slope.raster <- terrain(MNT.crop, opt='slope', neighbors=8, unit='degrees')
  # aspect - exposition
  #MNT.crop.resampled <- DeZoom(MNT.crop,rayon.exposition/4.5)
  aspect.raster <- terrain(MNT.crop, opt='aspect',neighbors=4,unit='degrees')
  # TPI = topographic position index, alti(i)-moyenne alt(i+rayon)
  # je fait un TPI avec alt moyenne invers?ment pond?r?e par la distance, afin de discriminer les zones plates localement mais globalement en fond de vall?e
  # le tpi c'est le plus long en temps de calcul, je dezoom avant sinon trop long:
  MNT.crop.resampled <- DeZoom(MNT.crop,2*resolution)
  
  pond.TPI.norm <- focalWeight(MNT.crop.resampled, d=rayon.tpi.norm, "circle")
  TPI.basic.raster <- MNT.crop.resampled-focal(MNT.crop.resampled, w=pond.TPI.norm, fun="sum", na.rm=T,pad=F)
  
  pond.TPI.norm <- focalWeight(MNT.crop.resampled, d=rayon.tpi.norm, "Gauss")
  TPI.norm.raster <- MNT.crop.resampled-focal(MNT.crop.resampled, w=pond.TPI.norm, fun=sum, na.rm=T,pad=F)
  # je part d'un filtre gaussien que je modifie
  pond.TPI <- focalWeight(MNT.crop.resampled, d=rayon.tpi.inv, "Gauss")
  # enlever les valeurs les plus basses (coins)
  pond.TPI[pond.TPI<quantile(pond.TPI,0.25)]<-quantile(pond.TPI,0.35)
  # inverser la pond?ration
  pond.TPI.inv <- 1/pond.TPI 
  pond.TPI.inv <- pond.TPI.inv /sum(pond.TPI.inv) 
  TPI.inv.raster <- MNT.crop.resampled-focal(MNT.crop.resampled, w=pond.TPI.inv, fun=sum, na.rm=T,pad=F)
  
  # calcul hillshade
  #MNT.crop.resampled <- DeZoom(MNT.crop,rayon.hillshade/4.5,method="bilinear")
  slope.raster.rad <- terrain(MNT.crop, opt='slope',neighbors=8,unit='radians')
  aspect.raster.rad <-terrain(MNT.crop, opt='aspect',neighbors=8,unit='radians')
  # hillshade modif 2019pour moyenne pendant période de végétation
  hill.ete <- hillShade(slope.raster.rad, aspect.raster.rad, 
                        angle=63.12, direction=180, normalize=T)
  
  # Hillshade au solstice d'hiver sous 180? d'angle solaire et 16.25? de hauteur solaire [0-255]
  #hill.hiver <- hillShade(slope.raster.rad, aspect.raster.rad, angle=16.25, direction=180, normalize=T)
  
  # Hillshade ? l'?quinoxe 20 mars sous 180 d'angle solaire et 39.52 de hauteur solaire [0-255]
  hill.mars <- hillShade(slope.raster.rad, aspect.raster.rad, 
                         angle=39.52, direction=180, normalize=T)
  
  # Hillshade ? l'?quinoxe 23 sept sous 180 d'angle solaire et 39.62 de hauteur solaire [0-255]
  hill.sept <- hillShade(slope.raster.rad, aspect.raster.rad, 
                         angle=39.62, direction=180, normalize=T)
  
  hillshade.raster <- mean(hill.mars ,hill.ete, hill.sept)
  
  # creation du raster de sortie
  SS.raster <- MNT.crop
  SS.raster[] <- 0 # met les valeurs ? z?ro
  
  # toutes les couches doivent avoir la m?me r?solution (attention, je n'ai pas mis le bilinear, plus rapide mais moins pr?cis)
  slope.raster <- resample(slope.raster,SS.raster)
  aspect.raster <- resample(aspect.raster,SS.raster)
  hillshade.raster <- resample(hillshade.raster,SS.raster)
  TPI.norm.raster <- resample(TPI.norm.raster,SS.raster)
  TPI.inv.raster <- resample(TPI.inv.raster,SS.raster)
  TPI.basic.raster <- resample(TPI.basic.raster,SS.raster)
  
  # cl? des sous-secteur selon Delvaux et Galoux [1962]
  
  # DetG : seuil entre 15 et 20 %, soit entre 8.5 et 11.3 --> grosse fourchette. on garde 8.5 com seuil, c'est d?j? bien pentu.
  class.slope <- cut(slope.raster, breaks=c(0,8.5,100)) #1 = peu de pente, 2= forte pente
  # cours de Hugues C (fait par FR): seuil de 292.5 et 112.5. DetG: 285 et 125. Je fait un buffer autour de ces limites, sous secteur m?sotherme mis en neutre/principal
  buff.aspect <- 0 # pour les situtation topographiques, je met un buffer de 0
  class.aspect <- reclassify(aspect.raster, c(125+buff.aspect,285-buff.aspect,1  ,125-buff.aspect,125+buff.aspect,3 ,285-buff.aspect,285+buff.aspect,3 ,0,125-buff.aspect,2, 285+buff.aspect,360,2)) #1 = chaud, 2= froid , 3 principal/neutre/m?sotherme
  class.hillshade <- cut(hillshade.raster,breaks=c(0,130,255)) # 1= zone ombrag?e
  class.tpi.inv <- cut(TPI.inv.raster, breaks=c(-Inf,-15,+Inf)) #1 = en vallon, 2=  position surr?lev? .. -30: les larges vallons dans une plaine, avec seulement 30 m de d?nivell? avec leur entourage, sont pas en secteur froid (lesse dans famenne et calestienne)
  class.tpi.norm <- cut(TPI.norm.raster, breaks=c(-Inf,-10,+Inf)) # vallon encaiss? 
  # ajout
  class.tpi.basic <- cut(TPI.basic.raster, breaks=c(-Inf,-5,-1,+Inf))
  
  # arbre de d?cision/classification
  
  # zone pentue
  SS.raster[class.slope[]==2 & class.aspect[]==1] <- 1 #SS chaud
  SS.raster[class.slope[]==2 & class.aspect[]==2] <- 2 #SS froid
  SS.raster[class.slope[]==2 & class.aspect[]==3] <- 3 #SS neutre/meso/principal
  
  # zone peu ?clair?e : partie inf?rieure toujour ombrag? des grands versants
  SS.raster[(class.slope[]==1 | class.aspect[]==3) & class.hillshade[]==1] <- 4 #SS froid
  
  # zone de fond de vall?e large en zone peu pentue
  SS.raster[class.slope[]==1 & class.tpi.inv[]==1 & slope.raster[]<3] <- 5 #SS froid
  
  # zone de fond de vall?e ?troite mais peu pentue
  SS.raster[class.slope[]==1 & class.tpi.norm[]==1 & class.tpi.inv[]!=1] <- 6 # fond de vall?e
  # zone de fond de vall?e ?troite et pentue 
  SS.raster[((class.slope[]==1 & slope.raster[]>3)|class.aspect[]==3) & class.tpi.norm[]==1 ] <- 6
  
  #SS.raster[alea.inondation.raster[]==1] <- 6
  # ajout 08 2016 : les fond de vallon encaiss? sont en exposition jusqu'en bas, c'est mal, cf bois de lairi
  SS.raster[class.tpi.norm[]==1 & class.tpi.basic[]!=3 & SS.raster[]==0] <- 7
  SS.raster[class.tpi.basic[]==1 & SS.raster[]==0] <- 7
  # compensation exposition par encaissement
  # on veux r?duire les ss chaud qui sinon descende bas dans les fond de vall?e. pour les situations topo, on ne peux pas faire cela uniquement pour SS chaud, pareil pour froid
  #SS.raster[class.tpi.norm[]==1 & class.tpi.basic[]!=3] <- 7
  #SS.raster[class.tpi.basic[]==1 & SS.raster[]==1] <- 7
  
  # sous secteur neutre: le reste
  SS.raster[SS.raster[]==0] <- 8 #SS neutre
  # enlever le bord, pour ?viter effet de bord
  SS.raster <- crop(SS.raster,zone)
  
  if(META==T & filename!=""){
    writeRaster(SS.raster, filename=paste(filename,"META_CLASSIF",sep="_"), format="GTiff",overwrite=T)
  }
  # attribuer les situations topographique
  
  # 1 versant nord 2 plateau et faible pente 3 versant sud 4 fond de vall?e ?troite 
  
  SS.raster <- reclassify(SS.raster, c(1,1,3,  2,2,1,  3,3,2  ,4,4,4 ,5,5,4,   6,6,4,  7,7,4 ,8,8,2 ),include.lowest=T, right=NA)
  # de changement de r?soltion s'effectue ? la fin, sinon perte de qualit?
  SS.raster <- DeZoom(SS.raster,resolution)
 
  SS.raster <- reclassify(SS.raster, c( 0,0.5,0, 0.5,1.5,1   ,1.5,2.5,2  ,2.5,3.5,3 ,3.5,4.5,4))
  
  # fin fonction
  if (filename==""){
    return(SS.raster)
  } else {
    writeRaster(SS.raster, filename=filename, format="GTiff",overwrite=T)
    cat(paste("fin carto des situations topographiques. \nVoir le Fichier ", getwd(),"//", filename,".tif qui contient les r?sultats. 1=ss froid, 2=ss neutre, 3=ss chaud \n",sep=""))
  }
  
  # purge
  rm(MNT,MNT.crop,MNT.crop.resampled,slope.raster,slope.raster.rad,aspect.raster,aspect.raster.rad,TPI.norm.raster,TPI.inv.raster,hillshade.raster,hill.ete,SS.raster)
  if(quietly==F){cat("Fin calcul des  situations topographiques \n")}
}





extract_SousSecteur <- function (xy=c(194000,129000)
                                 ,MNT.path = "D://Carto/MNT/MNT_10m_WALLONIA.tif"
                                 ,... )#ellipsis, permet de ne pas ?crite tout les argument qui peuvent ?tre pass? ? ma fonction
{
  
  # le point d'int?r?t
  pt <- SpatialPoints(matrix(data=xy, nrow=1, ncol=2))
  # une zone buffer
  zone <- gBuffer(pt,width=500)
  # sauver la zone pour qu'elle soit lue par apr?s par la fonction suivante
  filetmp <- paste(getwd(),"//tmp_joSousSecteur",sep="")
  
  df<- data.frame(id = getSpPPolygonsIDSlots(zone))
  row.names(df) <- getSpPPolygonsIDSlots(zone)
  zone <- SpatialPolygonsDataFrame(zone,data=df)
  writePolyShape(zone,filetmp)
  #writeOGR(zone,dsn=".", layer=filetmp, driver="ESRI Shapefile")
  
  SS.raster <- map_SousSecteur(shp.polyg.path=filetmp,...)                             
  SS <- extract(SS.raster,pt)
  
  # fin fonction
  return(SS)
}

# effectue un aggregate ou disaggregate pour arriver le plus proche possible d'une r?solution renseign?e par l'utilisateur
DeZoom <- function(raster,res= 10,method='',my.fun=mean){
  
  fact <- res/xres(raster)
  if(round(fact)>1){raster.resampled <- aggregate(raster, fact=round(fact),fun=my.fun)}else{raster.resampled <- disaggregate(raster, fact=round(1/fact), method=method)}
  return(raster.resampled)
}




extractCoords <- function(sp.df)
{
  results <- list()
  for(i in 1:length(sp.df@polygons[[1]]@Polygons))
  {
    results[[i]] <- sp.df@polygons[[1]]@Polygons[[i]]@coords
  }
  results <- Reduce(rbind, results)
  results
}

open_cnsw_tuiles <- function(zone)
{
  tuiles.shp.path <- "/home/lisein/virtualBoxSharing/NH_RW2019/data/CNSW_tuiles//grille50x50km.shp"
  tuiles <- readShapePoly(tuiles.shp.path)
  zone <- gUnaryUnion(zone)
  # determiner dans quelle tuiles se situe la zone
  tuile<- tuiles[which(gContains(tuiles,gBuffer(zone,width=-100), byid=T)),] # c(which(gOverlaps(zone, tuiles, byid=T)),
  #if(length(tuile)==0){ tuile <- tuiles[which(gOverlaps(tuiles,zone, byid=T)),]} retourne 2 dalle si la zone touche 2, ?a foire; je fait un buf n?gatif, voir ligne au dessus
  cnsw.tuile.name <- paste("/home/lisein/virtualBoxSharing/NH_RW2019/data/CNSW_tuiles//CNSW_tuile",tuile$ID,".shp",sep="")
  cat(cnsw.tuile.name)
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



open_hydro_tuiles <- function(zone)
{
  tuiles.shp.path <- "/home/lisein/virtualBoxSharing/NH_RW2019/data/CNSW_tuiles//grille50x50km.shp"
  tuiles <- readShapePoly(tuiles.shp.path)
  zone <- gUnaryUnion(zone)
  # determiner dans quelle tuiles se situe la zone
  tuile<- tuiles[which(gContains(tuiles,gBuffer(zone,width=-100), byid=T)),]    # c(which(gOverlaps(zone, tuiles, byid=T)),
  #if(length(tuile)==0){ tuile <- tuiles[which(gOverlaps(tuiles,zone, byid=T)),]} 
  hydro.tuile.name <- paste("/home/lisein/virtualBoxSharing/NH_RW2019/data/hydro_tuiles//hydro_tuile",tuile$ID,".shp",sep="")
  rds.name <- paste("/home/lisein/virtualBoxSharing/NH_RW2019/data//",tuile$ID,".rds",sep="")
  
  if (file.exists(rds.name)){ hydro <- readRDS(rds.name)
  return(hydro)
  }else{
    
    if (file.exists(hydro.tuile.name)){ hydro <- readShapeLines(hydro.tuile.name)
    saveRDS(hydro, file=rds.name)
    }else{cat(paste("le fichier ", hydro.tuile.name, " n'existe pas"))}
    
  }
    return(hydro)
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




