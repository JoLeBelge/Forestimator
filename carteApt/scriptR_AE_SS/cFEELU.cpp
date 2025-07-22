#include "cFEELU.h"


// réduire distance autour des petits cours d'eau (sinon ça foire tout le bazard) ; faire une carte des distances en ajoutant cours d'eau un à un

int nb_riv(12609+1);
//int nb_riv(500);
int step(nb_riv/50);
double GSD(2.5);

int c(0);



Fonc_Num lissage(Fonc_Num f)
{
    Im2D_REAL8 aDenoise55(5,5,
                          "1 4 7 4 1 "
                          "4 16 26 16 4 "
                          "7 26 41 26 7 "
                          "4 16 26 16 4 "
                          " 1 4 7 4 1"
                          );
    return (1/273)*som_masq(f,aDenoise55);
}

Fonc_Num TPI(Fonc_Num f,INT nb)
{
    Fonc_Num focal = 0;
    // double GSD=2.5;// taille cellulle
    for (INT x = -nb; x <= nb; x++)
        for (INT y = -nb; y <= nb; y++)
            focal = focal + trans(f,Pt2di(x,y));

    return f-(focal/ElSquare(2*nb+1));
}

Fonc_Num TPIpond(Fonc_Num f,INT nb)
{
    Fonc_Num focal = 0;
    double wtot(0);
    // double GSD=2.5;// taille cellulle
    for (INT x = -nb; x <= nb; x++){
        for (INT y = -nb; y <= nb; y++) {
            if (!((x==0) && (y==0))) {
                double w(1.0/euclid(Pt2di(x,y)));
                focal = focal + w*trans(f,Pt2di(x,y));
                wtot=wtot+w;
            }
        }
    }
    return f-(focal/wtot);
}

Fonc_Num focalBasic(Fonc_Num f,INT nb)
{
    Fonc_Num focal = 0;

    for (INT x = -nb; x <= nb; x++)
        for (INT y = -nb; y <= nb; y++)
            focal = focal + trans(f,Pt2di(x,y));
    return focal/ElSquare(2*nb+1);
}

Fonc_Num focalPondInv(Fonc_Num f,INT nb)
{
    Fonc_Num focal = 0;
    double wtot(0);
    for (INT x = -nb; x <= nb; x++){
        for (INT y = -nb; y <= nb; y++) {
            if (!((x==0) && (y==0)) && (euclid(Pt2di(x,y))<=nb)) {
                double w(1.0/euclid(Pt2di(x,y)));
                focal = focal + w*trans(f,Pt2di(x,y));
                wtot=wtot+w;
            }
        }
    }
    return focal/wtot;
}

Fonc_Num focalPond(Fonc_Num f,INT nb)
{
    Fonc_Num focal = 0;
    double wtot(0);
    for (INT x = -nb; x <= nb; x++){
        for (INT y = -nb; y <= nb; y++) {
            if (!((x==0) && (y==0)) && (euclid(Pt2di(x,y))<=nb)) {
                double w(euclid(Pt2di(x,y)));
                focal = focal + w*trans(f,Pt2di(x,y));
                wtot=wtot+w;
            }
        }
    }
    return focal/wtot;
}




Fonc_Num surfCurvIndex(Fonc_Num f,INT nb)
{
    Fonc_Num res = 0;
    for (INT x = -nb; x <= nb; x++)
        for (INT y = -nb; y <= nb; y++)
            // eviter division par 0
            if ((x!=0) | (y!=0)) res = res + (f-trans(f,Pt2di(x,y)))/euclid(Pt2di(x,y));
    //return res/ElSquare(2*nb+1);
    return res/ElSquare(2*nb);
}
// curvature index et upslope contributing area
//terrain caracterization index
Fonc_Num TCI(Fonc_Num aCurv,Fonc_Num aFA)
{
    Fonc_Num res = aCurv*log(aFA);

    return res;
}

// voir aussi wetness index




// a faire: repositionner le cours d'eau, pour qu'il suive le relief. ou plus simplement, attribuer une valeur de FA pour chaque pixel du tronçon.

// en partant du FA
// 1) en reliant l'amont et l'aval de chaque tronçon de rivière par un chemin dont le cout dépends de la distance au tronçon, de MNTrel, de FA
c_Appli_FEELU::c_Appli_FEELU(int argc,char ** argv)
{
    mTmpDir="metric4AE";
    mNameDelatHOut="../intermediate/MNTrel.tif";
    mNameDistOut="DistanceChamfer.tif";
    mNameAE="../output/AE.tif";
    mNameFlowAccOut="FAdilate.tif";
    mNameFlowAccIn="FA_250cm.tif";
    mNameLabel="Label.tif";
    mNameEcoulement="Ecoulement.tif";
    mTFWin="tmp.tfw";
    seuilDist=100;
    mFileEcoul="../intermediate/type_ecoul.csv";
    mForce=0;
    std::string aNameSurfEau="SurfEau_250cm.tif";
    mNameMNTrel2="../intermediate/MNTrel2.tif";
    mNameDist2="../intermediate/DistanceChanfer2.tif";
    mMode=0;
    mParam=3;

    mNameSlope="../intermediate/slope.tif";
    mNameTPI1="../intermediate/TPIbasic80m.tif";
    mNameTPI2="../intermediate/TPIpondInv80m.tif";
    mNameTPI3="../intermediate/TPIpondInv200m.tif";
    mNameTPI4="../intermediate/TPIpondInv2000m.tif";
    mNameInondation="Inondation_250cmInt16.tif";
    mNameAEcombine="../output/AEcombine.tif";
    mNameAEcleaned="../output/AEclean.tif";

    ElInitArgMain
            (
                argc,argv,
                LArgMain()   << EAMC(mNameMNT,"MNT tif file")
                << EAMC(mNameHydro,"hydrographic network; tif file with id of hydro line for each pixel")
                << EAMC(mNameFlowAccIn,"flow accumulation map")
                ,
                LArgMain()  << EAM(mNameDelatHOut,"Out",true, "Name of resulting map")
                << EAM(seuilDist,"Dist",true, "Distance from river to compute MNT rel, Flow Acc, in chamfer 32 distance" )
                << EAM(mDebug,"Debug",true, "Write intermediate results for debug purpose." )
                << EAM(mTmpDir,"TmpDir",true, "Directory for intermediate results generated in debug mode." )
                << EAM(mForce,"F",true, "force recomputation of intermediate results, def false." )
                << EAM(mMode,"Mode",true, "0; compute AE based on hydrologic input and processing, 1: AE only with topographie metrics and combine with hydro, 2: post processing of raster" )
                << EAM(mParam,"FParam",true, "one parameter for filter on MNT" )
                );

    if (!MMVisualMode)
    {

        // image de synthèse pour distance de chamfer autour d'un pt
        mFA=Im2D_REAL4::FromFileStd(mNameFlowAccOut);
        Im2D_INT1 aOut(mFA.sz().x,mFA.sz().y,0);
        // peut pas prendre log de 0 ou négatif
        ELISE_COPY(select(mFA.all_pts(),mFA.in(1)>0.0),log(mFA.in(1)),aOut.oclip());
        Tiff_Im::CreateFromIm(aOut,"FA_dilateLog.tif");



        mTmpDir=mDir+mTmpDir;

        if (!EAMIsInit(&mNameDelatHOut)) mNameDelatHOut=mDir+mNameDelatHOut;
        mICNM = cInterfChantierNameManipulateur::BasicAlloc(mDir);




        // filtre: partie non hydro de la détection apport eau
        if (mMode==1) {  //surfaceCurvatureIndex();


            mMNT=new Tiff_Im(Tiff_Im::StdConv(mNameMNT));
            sz=mMNT->sz();
            std::cout << "MNT : ok\n";
            //Im2D_REAL4 * aMNT=new Im2D_REAL4(1,1);
            Im2D_REAL4 * aMNT=new Im2D_REAL4(sz.x,sz.y);
            ELISE_COPY(mMNT->all_pts(),mMNT->in(),aMNT->out());
            std::cout << "MNT in ram memory\n";

            Im2D_INT2 aTPI4(sz.x,sz.y,0);
            loadOrCompute(mNameTPI4,aMNT,&aTPI4,20,2000,std::string("TPIpondInv"));

            // attention mémoire vive; 4x 1,7 G= 6,8+ le MNT + les produits intermédiaires; ça fait peter la mémoire vive!!

            // du coup je ne charge pas le MNT
            delete aMNT;
            aMNT=new Im2D_REAL4(1,1);

            Im2D_INT2 aSlope(sz.x,sz.y,0);
            loadOrCompute(mNameSlope,aMNT,&aSlope,4,20,std::string("slope"));

            Im2D_INT2 aTPI3(sz.x,sz.y,0);
            loadOrCompute(mNameTPI3,aMNT,&aTPI3,8,200,std::string("TPIpondInv"));


            //Im2D_INT2 aTPI1(sz.x,sz.y,0);
            //loadOrCompute(mNameTPI1,aMNT,&aTPI1,4,80,std::string("TPIbasic"));

            Im2D_INT2 aTPI2(sz.x,sz.y,0);
            loadOrCompute(mNameTPI2,aMNT,&aTPI2,4,80,std::string("TPIpondInv"));

            delete aMNT;
            std::cout << "Classif AE avec metric topo\n";
            Im2D_U_INT1 aAE(sz.x,sz.y,0);


            // meme métadonné que script R, pour rester cohérent
            // fond de grande vallée: ordre TPI 2000 et contre-ordre TPI 200 ;-)
            // ça va quasiment donné le shp inondation
            ELISE_COPY(select(aAE.all_pts(), aTPI4.in(0)<-200 && aSlope.in(0) < 40 && aTPI3.in(0) > -15 && aAE.in(0)==0), 1 , aAE.oclip());

            // voir cuvette à dondelange pour calib (seuil pente surtout)
            ELISE_COPY(select(aAE.all_pts(), aTPI3.in(0)<-100 && aSlope.in(0) < 95 && aAE.in(0)==0), 2 , aAE.oclip());

            // à nouveau, pente est difficile à seuiller
            ELISE_COPY(select(aAE.all_pts(), aTPI2.in(0)<-60  && aSlope.in(0) < 175 && aAE.in(0)==0), 4 , aAE.oclip());

            // perte
            // ajouter un critère de distance au cours d'eau permanent!!
            ELISE_COPY(select(aAE.all_pts(), ((aTPI2.in(0)>75) | ((aTPI2.in(0)>50) && (aSlope.in(0)>250))  | (aSlope.in(0)>425)) && aAE.in(0)==0), 6 , aAE.oclip());


            aSlope=Im2D_INT2(1,1,0);
            aTPI4=Im2D_INT2(1,1,0);
            aTPI2=Im2D_INT2(1,1,0);
            aTPI3=Im2D_INT2(1,1,0);

            std::string aName("../intermediate/AE_topo.tif");
            Tiff_Im::CreateFromIm(aAE,aName);
            ELISE_fp::copy_file(mTFWin,NameTFW(aName),1);

            std::cout << "combine with AE hydro\n";

            //if (ELISE_fp::exist_file(mNameDist2)){ mDistChanf=Im2D_U_INT1::FromFileStd(mNameDist2);}
            Im2D_U_INT1 aAEhydro=Im2D_U_INT1::FromFileStd(mNameAE);

            ELISE_COPY(select(aAE.all_pts(), aAE.in()>0 && aAE.in()<6 && aAEhydro.in(0)!=3), 2 , aAEhydro.oclip());

            ELISE_COPY(select(aAE.all_pts(), aAE.in()==6 && aAEhydro.in(0)==1), 0 , aAEhydro.oclip());


            Tiff_Im::CreateFromIm(aAEhydro,mNameAEcombine);
            ELISE_fp::copy_file(mTFWin,NameTFW(mNameAEcombine),1);

            std::cout << "done\n";
        }


        if (mMode==0) {


            std::cout << "lecture des couches d'entrée\n";
            mHydro=Im2D_U_INT2::FromFileStd(mNameHydro);
            std::cout << "Hydro: ok\n";

            sz=mHydro.sz();
            // cette liste de pt permet de sauver du temps dans les selections de pixels
            lpHydro= new Liste_Pts_INT2(2);
            //lpHydro(2);
            ELISE_COPY(select(mHydro.all_pts(), mHydro.in()!=0),
                       666,
                       *lpHydro
                       );

            //______________________________________________________________________
            // c'est juste pour faire des statistique en dehors des traitement d'images
            if ( 0 && ELISE_fp::exist_file(mNameFlowAccOut)){
                std::cout << "Extract FA value for each river and save in txt file\n";
                readVal4EachRiver(std::string("FA.txt"));}
            //______________________________________________________________________
            // distance au réseau hydro
            if (ELISE_fp::exist_file(mNameDistOut) | !mForce){
                mDistChanf=Im2D_U_INT1::FromFileStd(mNameDistOut);
                std::cout << "Distance au réseau hydro : ok\n";
            } else {

                std::cout << " calcul dist de chamfer du reseau hydro\n";
                mDistChanf=Im2D_U_INT1(sz.x,sz.y,1);
                ELISE_COPY(lpHydro->all_pts(),
                           0,
                           mDistChanf.out()
                           );
                ChamferNoBorder(&mDistChanf);

                Tiff_Im::CreateFromIm(mDistChanf,mNameDistOut);
                ELISE_fp::copy_file(mTFWin,NameTFW(mNameDistOut),1);
                std::cout << " ok\n";
            }


            /*
        if (!ELISE_fp::exist_file(mNameLabel) | mForce){
            std::cout << "carte des labels\n";
            //computeDistMap(); trop lent en l'état
            mLabel=mHydro;
            _dilate(&mLabel,mDistChanf,bool(FALSE));
            Tiff_Im::CreateFromIm(mLabel,mNameLabel);
            ELISE_fp::copy_file(mTFWin,NameTFW(mNameLabel),1);
            std::cout << " ok\n";
            ;}
            */

            //mLabel=Im2D_U_INT2::FromFileStd(mNameLabel);
            mLabel    = new Tiff_Im(Tiff_Im::StdConv(mNameLabel));
            std::cout << "labels :  OK\n";


            mSurf=Im2D_U_INT2::FromFileStd(aNameSurfEau);
            //mSurf = new Tiff_Im(Tiff_Im::StdConv(aNameSurfEau));
            std::cout << "Surface Eau: ok\n";


            if (!ELISE_fp::exist_file(mNameMNTrel2) | !ELISE_fp::exist_file(mNameDist2) | mForce){
                // j'en ai besoin pour les surfaces - correction MNTrel
                //mMNT=Im2D_REAL4::FromFileStd(mNameMNT);
                mMNT=new Tiff_Im(Tiff_Im::StdConv(mNameMNT));
                std::cout << "MNT : ok\n";
                if (!ELISE_fp::exist_file(mNameDelatHOut) | mForce){
                    std::cout << "Je vais calculer le MNT relatif\n";
                    //mMNT=Im2D_REAL4::FromFileStd(mNameMNT);
                    std::cout << "MNT : ok\n";
                    computeMNTrel();
                }

                mMNTrel=Im2D_REAL4::FromFileStd(mNameDelatHOut);
                std::cout << "MNTrel : ok\n";

                //______________________________________________________________________
                std::cout << " passage de ligne réseau hydro à surface hydro (dilate avec MNT rel et intégration surf eau)\n";
                Im2D_U_INT1 * aDist= new Im2D_U_INT1(sz.x,sz.y,1);
                ELISE_COPY(select(mMNTrel.all_pts(),(mMNTrel.in()<1.0) &&( mDistChanf.in()<20) ),
                           0,
                           aDist->out()
                           );
                // ajout surface eau
                ELISE_COPY(select(mSurf.all_pts(),mSurf.in()>0),
                           0,
                           aDist->out()
                           );
                ChamferNoBorder(aDist);
                // je peux pas faire ça ici!! j'ai besoin des deux distances

                //mNameDistOut="DistSurface.tif";
                //Tiff_Im::CreateFromIm(mDistChanf,mNameDistOut);
                //ELISE_fp::copy_file(mTFWin,NameTFW(mNameDistOut),1);
                //std::cout << " ok\n";
                // mise à jour du MNTrel pour integrer les surface d'eau qui ne sont pas jointive du réseau hydro:j'ai besoin de 2 carte de distance, mDist et aDistSurf, et de mSurf
                updateMNTrel4SurfaceEau(aDist);
                // pas élégant mais j'ai changé les val de mSurf donc je recharge car j'en ai besoin pour carte ecoulements
                mSurf=Im2D_U_INT2::FromFileStd(aNameSurfEau);

                Tiff_Im::CreateFromIm(mMNTrel,mNameMNTrel2);
                ELISE_fp::copy_file(mTFWin,NameTFW(mNameMNTrel2),1);
            } else {

                mMNTrel=Im2D_REAL4::FromFileStd(mNameMNTrel2);
                std::cout << "MNTrel corrigé: ok\n";
                mDistChanf=Im2D_U_INT1::FromFileStd(mNameDist2);
                std::cout << "DistChanfer corrigé: ok\n";
            }

            if (!ELISE_fp::exist_file(mNameFlowAccOut) | mForce){
                std::cout << "Je vais calculer le FA dilaté\n";
                mFA=Im2D_REAL4::FromFileStd(mNameFlowAccIn);
                std::cout << "FA : ok\n";
                computeFA();
            }

            // écoulements permanents------------------------------------------------
            if (!ELISE_fp::exist_file(mNameEcoulement) | mForce){
                computeEcoulement();
                ;} else {
                std::cout << "Ecoulement permanent; ok\n";
                mEcoul=Im2D_U_INT1::FromFileStd(mNameEcoulement);}
            // écoulements permanents------------------------------------------------
            // j'ai plus besoin des surface , est libéré dans computeEcoulement() mais pas si la couche Ecoulement existait déjà..
            mSurf=Im2D_U_INT2(1,1,1);


            mFA=Im2D_REAL4::FromFileStd(mNameFlowAccOut);
            std::cout << "FA : ok\n";

            // liberer un peu d'espace et charger les derniers bazard
            mHydro=Im2D_U_INT2(1,1,0);
            //a ben non c'est une image tiff le mLabel mLabel=Im2D_U_INT2(1,1,0);
            Im2D_U_INT1 aInond=Im2D_U_INT1::FromFileStd(mNameInondation);
            std::cout << "Inondation: ok\n";

            Im2D_U_INT1 aAE(sz.x,sz.y,1);
            Liste_Pts_INT2 lpSeuilDist(2);
            ELISE_COPY(select(mDistChanf.all_pts(), mDistChanf.in()<seuilDist), 666, lpSeuilDist);

            std::cout << "Determiner Apport en eau sur base de tout le tralalala\n";

            //equation pour AE variable V1
            double va1(2.247775 ),vb1(-0.012942);
            //y=a+exp(b*x);
            double pa1(va1), pb1(vb1);
            // 2 catégorie de buffer en fonction de la classe de FA
            double pa2(2.0632), pb2(-0.028894);

            double seuilFA1(70000.0);

            // condition avec 3 facteurs; MNTrel, FA, distance
            ELISE_COPY(select(lpSeuilDist.all_pts(), (mMNTrel.in(100.0)<(va1+exp(vb1*mDistChanf.in(255))))),
                       2,
                       aAE.oclip()
                       );
            ELISE_COPY(select(lpSeuilDist.all_pts(),(mFA.in(seuilFA1)>seuilFA1) && (mEcoul.in(1)==0) && (mMNTrel.in(100.0)<(pa1+exp(pb1*mDistChanf.in(255))))),
                       3,
                       aAE.oclip()
                       );
            ELISE_COPY(select(lpSeuilDist.all_pts(), (mEcoul.in(1)==0) && (mMNTrel.in(100.0)<(pa2+exp(pb2*mDistChanf.in(255))))),
                       3,
                       aAE.oclip()
                       );

            // zone d'inondation pour grandes vallées
            ELISE_COPY(select(aAE.all_pts(),aAE.in(0)==1 && aInond.in(0)==1),2,aAE.oclip());

            Tiff_Im::CreateFromIm(aAE,mNameAE);
            ELISE_fp::copy_file(mTFWin,NameTFW(mNameAE),1);

        }

        if (mMode==2) {

            std::cout << "Clean raster\n";
            if (ELISE_fp::exist_file(mNameAEcombine)){
                Im2D_U_INT1 aAE=Im2D_U_INT1::FromFileStd(mNameAEcombine);
                clean(&aAE);
                Tiff_Im::CreateFromIm(aAE,mNameAEcleaned);
                ELISE_fp::copy_file(mTFWin,NameTFW(mNameAEcleaned),1);
                std::cout << "done\n";

            }
        }
    }

}


void c_Appli_FEELU::TPIbyResampling(Im2D_REAL4 * aMNT,Im2D_INT2 * aOut, int aFactR, int aDist,std::string aType){
    std::cout << "TPI by resampling...\n";
    Pt2di aSzR = round_up(Pt2dr(aMNT->sz())/aFactR);
    int aNbCell4focal=round_up(double(aDist)/(2.5*aFactR));

    Im2D_REAL4 aResampledMNT(aSzR.x,aSzR.y,0.0);
    Fonc_Num aFIn = StdFoncChScale(aMNT->in_proj(),Pt2dr(0,0),Pt2dr(aFactR,aFactR));
    ELISE_COPY(aResampledMNT.all_pts(),aFIn,aResampledMNT.out());
    std::cout << "Resampling fact " << aFactR << " : ok\n";

    Im2D_REAL4 aFocal(aSzR.x,aSzR.y,0.0);

    Fonc_Num FIN;
    if (aType=="TPIpondInv") FIN=focalPondInv(aResampledMNT.in_proj(),aNbCell4focal);
    if (aType=="TPIpond") FIN=focalPond(aResampledMNT.in_proj(),aNbCell4focal);
    if (aType=="TPIbasic") FIN=focalBasic(aResampledMNT.in_proj(),aNbCell4focal);
    ELISE_COPY(aFocal.all_pts(),FIN,aFocal.out());

    std::cout << "Focal with dist (cell) " << aNbCell4focal << " : ok\n";

    Fonc_Num Result = (aMNT->in()-StdFoncChScale(aFocal.in(0),Pt2dr(0,0),Pt2dr(1.0/aFactR,1.0/aFactR)));

    // Iconv retourne un integer long, sur 4 octet. moi je suis sur entier court, 2 octet=1,7 G par image
    // dommage de devoir faire une image en mémoire juste pour le cast mais le changement de scale plus condition en mm temps c'est trop, bug
    Im2D_REAL4 aResultReal(aMNT->sz().x,aMNT->sz().y,0.0);
    ELISE_COPY(aResultReal.all_pts(),Result,aResultReal.oclip());
    std::cout << "passe par image real4 avant cast en integer : ok\n";
    ELISE_COPY(select(aOut->all_pts(),Abs(aResultReal.in(0))<1000.0),Iconv(10*aResultReal.in()),aOut->oclip());
    std::cout << "done\n";
}


void c_Appli_FEELU::slope(Im2D_REAL4 * aMNT, Im2D_INT2 *aOut, int aFactR, int aDist){

    std::cout << "slope...\n";

    Pt2di aSzR = round_up(Pt2dr(aMNT->sz())/aFactR);
    int nb=round_up(double(aDist)/(2.5*aFactR));

    Im2D_REAL4 aResampledMNT(aSzR.x,aSzR.y,0.0);
    //Fonc_Num aFIn = StdFoncChScale(lissage(aMNT->in_proj()),Pt2dr(0,0),Pt2dr(aFactR,aFactR));
    Fonc_Num aFIn = StdFoncChScale(aMNT->in_proj(),Pt2dr(0,0),Pt2dr(aFactR,aFactR));
    ELISE_COPY(aResampledMNT.all_pts(),aFIn,aResampledMNT.out());
    std::cout << "Resampling fact " << aFactR << " : ok\n";

    ELISE_COPY(aResampledMNT.all_pts(),aResampledMNT.in(),aResampledMNT.out());

    Im2D_INT2 aSlope(aSzR.x,aSzR.y,0.0);
    REAL4 ** d = aResampledMNT.data();
    for (INT x=nb; x < aResampledMNT.sz().x-nb; x++)
    {
        for (INT y=nb; y < aResampledMNT.sz().y-nb; y++)
        {
            double penteMax(0.0);
            // boucle sur les voisin les plus proche
            for (INT x2 = -nb; x2 <= nb; x2++){
                for (INT y2 = -nb; y2 <= nb; y2++) {
                    int i(x+x2),j(y+y2);
                    double aDxy(euclid(Pt2di(x2,y2))*aFactR*GSD);
                    if (!((x2==0) && (y2==0)) && (aDxy<=aDist)) {

                        double pente=atan((ElAbs(d[j][i]-d[y][x]))/aDxy);
                        if (pente>penteMax) penteMax=pente;
                    }
                }
            }

            aSlope.SetI(Pt2di(x,y),10.0*penteMax*(180.0/PI));
        }
    }
    std::cout << "Slope with dist (cell) " << nb << " : ok\n";
    ELISE_COPY(aOut->all_pts(),StdFoncChScale(aSlope.in(0),Pt2dr(0,0),Pt2dr(1.0/aFactR,1.0/aFactR)),aOut->oclip());
    std::cout << "done\n";
}


void c_Appli_FEELU::loadOrCompute(std::string aName,Im2D_REAL4 * aMNT, Im2D_INT2 *aOut, int aFactR, int aDist,std::string aType){

    if (ELISE_fp::exist_file(aName)){
        Tiff_Im aTmp(Tiff_Im::StdConv((aName)));
        ELISE_COPY(aOut->all_pts(),aTmp.in(0),aOut->oclip());
        std::cout << aType << " : ok\n";
    } else {

        if (aType=="TPIbasic") TPIbyResampling(aMNT,aOut,aFactR,aDist,aType);
        if (aType=="TPIpondInv") TPIbyResampling(aMNT,aOut,aFactR,aDist,aType);
        if (aType=="TPIpond") TPIbyResampling(aMNT,aOut,aFactR,aDist,aType);
        if (aType=="slope") slope(aMNT,aOut,aFactR,aDist);

        Tiff_Im::CreateFromIm(*aOut,aName);
        ELISE_fp::copy_file(mTFWin,NameTFW(aName),1);
        std::cout << aType << " : ok\n";
    }
}


void c_Appli_FEELU::clean(Im2D_U_INT1 *aIn){

    int Val2Clean(3),ValConflict1(10),ValCopain(10),seuilVois(5);
    // Val Conflict: les classe pour lesquelles ont ne veux pas de filtrage: ex filtrage de perte, ne peux pas toucher à apport variable
    // Val Copain; les classes qui sont assimilée à celles à nettoyer; ex app variable (Val to Clean) a pour copain les apports permanents: un pixels sans apport au milieu de Apport variable et Apport permanent va passer en apport variable

    std::cout << "clean apports permanent\n";
    for (int iter(1); iter <5;iter++){
    // 49-25 = 24.
    seuilVois=14;
    fillHole(aIn,Val2Clean,ValCopain,ValConflict1,seuilVois,int(3),int(2));
    // 9pow 2= 81 - 5pow2=56
    seuilVois=30;
    fillHole(aIn,Val2Clean,ValCopain,ValConflict1,seuilVois, int(4),int(2));
    }

    cleanVoisinage(aIn,Val2Clean,ValConflict1,int(5));
    // chasse au pixels isolés AE permanent - les passe en variable
    for (int iter(1); iter <3;iter++){cleanIsolatedPix(aIn,int (3),int(2),int(2));}


    std::cout << "clean apports variables\n";
    Val2Clean=2;
    ValConflict1=3;
    ValCopain=3;
    for (int iter(1); iter <5;iter++){
    // 49-25 = 24.
    seuilVois=14;
    fillHole(aIn,Val2Clean,ValCopain,ValConflict1,seuilVois,int(3),int(2));
    // 9pow 2= 81 - 5pow2=56
    seuilVois=30;
    fillHole(aIn,Val2Clean,ValCopain,ValConflict1,seuilVois, int(4),int(2));
    }

    cleanVoisinage(aIn,Val2Clean,ValConflict1,int(5));
    // chasse au pixels isolés AE variable - les passe en pas d'apport
    for (int iter(1); iter <3;iter++){cleanIsolatedPix(aIn,int (2),int(1),int(2));}

    std::cout << "clean pertes en eau\n";
    Val2Clean=0;
    ValConflict1=2;
    ValCopain=0;
    for (int iter(1); iter <5;iter++){
    // 25-0 = 24.
    seuilVois=13;
    // échelle plus fine car plus petite zone que AE perm et var
    fillHole(aIn,Val2Clean,ValCopain,ValConflict1,seuilVois,int(2),int(0));
    }

    cleanVoisinage(aIn,Val2Clean,ValConflict1,int(5));
    // chasse au pixels isolés AE pertes - les passe en pas d'apport
    for (int iter(1); iter <3;iter++){cleanIsolatedPix(aIn,int (0),int(1),int(2));}


    /*
    seuilVois=5;
    for (int iter(1); iter <5;iter++){
     cleanVoisinage(aIn,Val2Clean,ValConflict1,seuilVois);
    }

    // pas de clean avec 4 voisin sur AE variable: sinon dilate indésirable sur les petits cours d'eau
    Val2Clean=2;
    ValConflict1=3;
    seuilVois=5;
    for (int iter(1); iter <5;iter++){
     cleanVoisinage(aIn,Val2Clean,ValConflict1,seuilVois);
    }

    Val2Clean=0;
    ValConflict1=2;
    seuilVois=4;
    for (int iter(1); iter <5;iter++){
     cleanVoisinage(aIn,Val2Clean,ValConflict1,seuilVois);
    }

    */

    //Neighbourhood V8=Neighbourhood::v8();
    //int VM(5);
    // pixels de la catégorie en question
    //ELISE_COPY(select(aIn->all_pts(),aIn->in(4)==Val2Clean),0,Im.oclip());

    //Liste_Pts_INT2 lpOut(2);

    /*U_INT1 ** d = Im.data();
    //U_INT1 ** nbVois = ImNbVois->data();

    for (INT x=0; x < aIn->sz().x; x++)
    {
        for (INT y=0; y < aIn->sz().y; y++)
        {
            if (d[y][x] == 1)
            {
                Liste_Pts_INT2 lpTmp(2);
                ELISE_COPY
                        (
                            conc
                            (
                                Pt2di(x,y),
                                Im.neigh_test_and_set(V8, 1, 0, VM) ),
                            666,
                            lpTmp
                            );
                // ceux de cette liste qui ont plue de X voisin, on les change de catégorie
                ELISE_COPY(select(lpTmp.all_pts(),ImNbVois.in()>=seuilVois),Val2Clean,aIn->oclip());
            }
        }
    }
    */
}

void c_Appli_FEELU::cleanVoisinage(Im2D_U_INT1 * aIn,int Val2Clean, int ValConflict1, int seuilVois){

    std::cout << "Nettoyage carte AE valeur " << Val2Clean << ", seuil nombre de voisin " << seuilVois << " Apport d'eau à ne pas modifier : " << ValConflict1 << std::endl;

    Im2D_U_INT1 Im(aIn->sz().x,aIn->sz().y,0);
    // pixels qu'on va peut-être changer de catégorie
    ELISE_COPY(select(aIn->all_pts(),aIn->in(0)!=Val2Clean && aIn->in(0)!=ValConflict1),1,Im.oclip());
    //statistique sur le nombre de pixel voisin qui sont de la classe d'AE à nettoyer
    Im2D_U_INT1 ImNbVois(aIn->sz().x,aIn->sz().y,0);
    Im2D_BIN * IbinTmp=new Im2D_BIN(aIn->sz().x,aIn->sz().y,0);
    ELISE_COPY(select(aIn->all_pts(),aIn->in(4)==Val2Clean),1,IbinTmp->oclip());
    ELISE_COPY(aIn->all_pts(),rect_som(IbinTmp->in(0),1),ImNbVois.oclip());
    delete IbinTmp;
    ELISE_COPY(select(aIn->all_pts(),ImNbVois.in()>=seuilVois && Im.in()==1),Val2Clean,aIn->oclip());
}

void c_Appli_FEELU::cleanIsolatedPix(Im2D_U_INT1 * aIn,int Val2Clean, int Val2Replace, int seuilVois){

    std::cout << "Nettoyage carte AE valeur " << Val2Clean << " pour pixels isolé, seuil nombre de voisin " << seuilVois << ", remplace par : " << Val2Replace << std::endl;

    Im2D_U_INT1 Im(aIn->sz().x,aIn->sz().y,0);
    // pixels qu'on va peut-être changer de catégorie
    ELISE_COPY(select(aIn->all_pts(),aIn->in(0)==Val2Clean),1,Im.oclip());
    //statistique sur le nombre de pixel voisin qui sont de la classe d'AE à nettoyer
    Im2D_U_INT1 ImNbVois(aIn->sz().x,aIn->sz().y,0);
    Im2D_BIN * IbinTmp=new Im2D_BIN(aIn->sz().x,aIn->sz().y,0);
    ELISE_COPY(select(aIn->all_pts(),aIn->in(4)==Val2Clean),1,IbinTmp->oclip());
    ELISE_COPY(aIn->all_pts(),rect_som(IbinTmp->in(0),1),ImNbVois.oclip());
    delete IbinTmp;
    // seuil voisin +1 car le pixels en question est compté dedans car de la mm catégorie mais n'est pas voisin de lui-même
    ELISE_COPY(select(aIn->all_pts(),ImNbVois.in()<=seuilVois+1 && Im.in()==1),Val2Replace,aIn->oclip());
}

void c_Appli_FEELU::fillHole(Im2D_U_INT1 * aIn,int Val2Clean, int ValCopain,int ValConflict1, int seuilVois,int aSz1,int aSz2){

    std::cout << "Nettoyage carte AE valeur " << Val2Clean << ", soutenu par valeur " << ValCopain <<", seuil nombre de voisin " << seuilVois << " Apport d'eau à ne pas modifier : " << ValConflict1 << std::endl;

    Im2D_U_INT1 Im(aIn->sz().x,aIn->sz().y,0);
    // pixels qu'on va peut-être changer de catégorie
    ELISE_COPY(select(aIn->all_pts(),aIn->in(0)!=Val2Clean && aIn->in(0)!=ValConflict1),1,Im.oclip());
    //statistique sur le nombre de pixel voisin qui sont de la classe d'AE à nettoyer
    Im2D_U_INT1 ImNbVois(aIn->sz().x,aIn->sz().y,0);
    Im2D_BIN * IbinTmp=new Im2D_BIN(aIn->sz().x,aIn->sz().y,0);
    ELISE_COPY(select(aIn->all_pts(),(aIn->in(4)==Val2Clean)| (aIn->in(4)==ValCopain)),1,IbinTmp->oclip());

    // 49 pixels de voisinage (3+1+3) pow 2
    ELISE_COPY(aIn->all_pts(),rect_som(IbinTmp->in(0),aSz1)-rect_som(IbinTmp->in(0),aSz2),ImNbVois.oclip());
    delete IbinTmp;
    ELISE_COPY(select(aIn->all_pts(),ImNbVois.in()>=seuilVois && Im.in()==1),Val2Clean,aIn->oclip());
}


void c_Appli_FEELU::surfaceCurvatureIndex(){


    std::string  aName("Curvature.tif");

    if (!ELISE_fp::exist_file(aName)) {
        std::cout << "Filtre sur MNT; surface curvature index\n";
        mMNT=new Tiff_Im(Tiff_Im::StdConv(mNameMNT));
        sz=mMNT->sz();
        std::cout << "MNT : ok\n";
        Im2D_REAL4 aC(sz.x,sz.y,0.0);
        Im2D_REAL4 * aMNT=new Im2D_REAL4(sz.x,sz.y);
        ELISE_COPY(mMNT->all_pts(),mMNT->in(),aMNT->out());
        std::cout << "MNT in ram memory\n";
        ELISE_COPY(aMNT->all_pts(),surfCurvIndex(aMNT->in_proj(), mParam),aC.oclip());
        Tiff_Im::CreateFromIm(aC,aName);
        ELISE_fp::copy_file(mTFWin,NameTFW(aName),1);
        std::cout << "done\n";

    } else {
        Im2D_REAL4 aC=Im2D_REAL4::FromFileStd(aName);
        std::cout << "Curvature : ok\n";

    }
}


void c_Appli_FEELU::computeEcoulement(){

    std::cout << "Je vais calculer la carte des écoulements permanents (val 0), qui integre aussi les surfaces d'eau\n";
    std::map<int,std::vector<std::string>> aRiver=parseCSV(mFileEcoul,',');
    mEcoul=Im2D_U_INT1(sz.x,sz.y,1);
    // ajout des surfaces d'eau: uniquement celle jointive du réseau hydro permanent!! sinon petit lac en plateau; une zone de 200 metre passe en écoulement permanent, ça fait tache
    Im2D_U_INT1 Ibin= Im2D_U_INT1(sz.x,sz.y,0);

    //Im2D_BIN EcouTempo= Im2D_BIN(sz.x,sz.y,0);
    Liste_Pts_INT2 lpSeuilDist(2);
    ELISE_COPY(select(mHydro.all_pts(), mDistChanf.in()<seuilDist), 666, lpSeuilDist );
    Im2D_U_INT2 * aLabel=new Im2D_U_INT2(sz.x,sz.y);
    ELISE_COPY(mLabel->all_pts(),aLabel->in(),aLabel->out());
    int pct(0), nb_rivP(0);
    for (auto & riv : aRiver){
        if (mod(riv.first,step)==0){
            pct=pct+2;
            std::cout << pct << "% " << std::endl;usleep(10);
        }

        if (std::stoi(riv.second.at(0))==0){
            nb_rivP++;
            //std::cout << "rivier " << riv.first << " a un ecoulement de " << riv.second.at(1) << " soit de " << std::stoi(riv.second.at(1)) << std::endl;
            ELISE_COPY(select(lpHydro->all_pts(),mHydro.in(0)==riv.first),
                       0,
                       mEcoul.oclip()
                       );
        } else {
            ELISE_COPY(select(lpHydro->all_pts(),mHydro.in(0)==riv.first),
                       2,
                       Ibin.oclip()
                       );

        }
    }
    std::cout << " trouvé " << nb_rivP << " elem du réseau hydro avec écoulement perm " << std::endl;
    delete aLabel;

    ELISE_COPY(select(lpSeuilDist.all_pts(),mSurf.in(0)>0),
               1,
               Ibin.oclip());
    // selection des composantes connexes sur mSurfEau

    U_INT1 ** d = Ibin.data();
    int count(0),tot(0); // counter
    Neighbourhood V8=Neighbourhood::v8();

    for (INT x=0; x < Ibin.sz().x; x++)
    {
        for (INT y=0; y < Ibin.sz().y; y++)
        {
            if (d[y][x] == 1)
            {
                tot++;
                Liste_Pts_INT2 cc1(2);
                ELISE_COPY
                        (
                            conc
                            (
                                Pt2di(x,y),
                                Ibin.neigh_test_and_set(V8, 1, 0,  20) ),
                            666,
                            cc1
                            );
                // vérif que la surface d'eau est jointive du réseau hydro permanent
                int test(0);
                ELISE_COPY
                        (
                            select(cc1.all_pts(),mEcoul.in(1)==0),
                            666,
                            sigma(test)
                            );
                // ajout à la carte des écoulements permanents.
                if (test>0){
                    count++;
                    ELISE_COPY(
                                cc1.all_pts(),
                                0,
                                mEcoul.oclip()
                                );
                }
            }
        }
    }


    std::cout << "Nombre de Surface d'eau  dans réseau hydro permanent: " << count << " sur " <<tot << std::endl;

    // moyen pour libérer de la RAM

    mSurf=Im2D_U_INT2(1,1,1);


    // maintenant dilate. je prend la distance que des cours d'eau permanent et surf en eau plutôt que la distance de tout les cours d'eau
    Im2D_U_INT1 aImDist(sz.x,sz.y,1);
    Im2D_U_INT1 aImDistTempo(sz.x,sz.y,1);
    //ELISE_COPY(select(lpHydro->all_pts(),mEcoul.in(1)==0), je peux plus utiliser la liste de pt, car j'ai ajouté les surfaces d'eau
    ELISE_COPY(select(mEcoul.all_pts(),mEcoul.in(1)==0),
               0,
               aImDist.oclip()
               );
    ELISE_COPY(select(Ibin.all_pts(),Ibin.in(1)==2),
               0,
               aImDistTempo.oclip()
               );

    Ibin=Im2D_U_INT1(1,1,0);

    ChamferNoBorder(&aImDist);
    ChamferNoBorder(&aImDistTempo);

    std::cout << " dilate carte des ecoulements permanent " << std::endl;

    _dilate(&mEcoul,aImDist,int(1),bool(0));

    // je dois vérifier que je ne dilate pas sur une zone d'écoulement temporaire; je peux faire cela avec la carte des labels? comment faire pour les surface d'eau qui ne sont pas dans la carte label??
    // supprime les zones qui sont dans le réseau temporaire
    //normalement aImDist devrai etre égal à mDistChanf mais il y a un petit décalge (bug) donc je met 5 de marge.
    for (INT x=0; x < mEcoul.sz().x; x++)
    {
        for (INT y=0; y < mEcoul.sz().y; y++)
        {
            Pt2di pos(x,y);
            int diff(aImDistTempo.GetI(pos)-aImDist.GetI(pos));
            if (diff<-20) {mEcoul.SetI(pos,1);}
        }
    }

    Tiff_Im::CreateFromIm(mEcoul,mNameEcoulement);
    ELISE_fp::copy_file(mTFWin,NameTFW(mNameEcoulement),1);
    std::cout << " ok\n";

}

void c_Appli_FEELU::updateMNTrel4SurfaceEau(Im2D_U_INT1 * aDistSurf){
    std::cout << "update MNTrel pour surface eau qui sont trop grande ou trop loin\n";
    //detection des surface d'eau pour lesquelles le MNTrel nécessite une MAJ = celle qui sont trop loin du réseau hydro ou celle qui sont trop grande
    Im2D_U_INT1 Ibin= Im2D_U_INT1(sz.x,sz.y,0);
    Im2D_U_INT1 Surf2Use(sz.x,sz.y,255);
    ELISE_COPY(select(aDistSurf->all_pts(),aDistSurf->in()==0 && mDistChanf.in()>100),
               1,
               Ibin.out());
    // selection des composantes connexes sur mSurfEau

    // je n'ai plus besoin des distances au réseau hydro, j'ai celles hydro + surface eau: plus complêt
    mDistChanf=*aDistSurf;
    Tiff_Im::CreateFromIm(mDistChanf,mNameDist2);
    ELISE_fp::copy_file(mTFWin,NameTFW(mNameDist2),1);
    delete aDistSurf;

    U_INT1 ** d = Ibin.data();
    int count(0); // counter
    Neighbourhood V8=Neighbourhood::v8();
    int VM(5275+1); // vmax pour le test and set , sert à Elise pour dimensionner des tables temporaires. c'est la valeur max des id surf eau

    Liste_Pts_INT2 LpSurf(2);
    for (INT x=0; x < Ibin.sz().x; x++)
    {
        for (INT y=0; y < Ibin.sz().y; y++)
        {
            if (d[y][x] == 1)
            {
                // on vérifie que l'id de la surface n'est pas à 0 (pour les grande surface qui sont coupée en plusieurs cc qui respectent plusieures fois le critère de sélection.
                int id_Surf(mSurf.GetR(Pt2di(x,y)));
                if (id_Surf!=0){
                    count++;

                    // std::cout << "Surface d'eau numéro " << count << " , label " << id_Surf<< "\n" << std::endl;
                    // selectionne la cc sur Ibin pour ne pas que la zone revienne dans la boucle
                    Liste_Pts_INT2 cc1(2);
                    ELISE_COPY
                            (
                                conc
                                (
                                    Pt2di(x,y),
                                    Ibin.neigh_test_and_set(V8, 1, 0,  20) ),
                                666,
                                cc1
                                );
                    // selectionne la zone sur mSurf - c'est elle qu'on va garder - et qu'on sauve dans surf2Use sur laquelle on calculera les distances

                    ELISE_COPY
                            (
                                conc (Pt2di(x,y),
                                      mSurf.neigh_test_and_set(V8, id_Surf, 0,VM  )),
                                0,
                                Surf2Use.oclip()|LpSurf
                                );
                }
            }
        }
    }

    std::cout << "Nombre de Surface d'eau : " << count << std::endl;

    // moyen pour libérer de la RAM
    Ibin=Im2D_U_INT1(sz.x,sz.y,0);
    mSurf=Im2D_U_INT2(1,1,1);

    std::cout << "Selection des surfaces: ok. calcul distance...\n";

    ELISE_COPY(select(Surf2Use.all_pts(),Surf2Use.in()==0),1,Ibin.out());
    computeDistMap1by1(&Ibin,&Surf2Use);
    Ibin=Im2D_U_INT1(1,1,1);

    Tiff_Im::CreateFromIm(Surf2Use,std::string("DistSurf2use.tif"));
    // ici, je pourrais encore relativement facilement ajuster les distances en fonction de la taille des surface d'eau
    // afin de limiter le problème des petites surf d'eau en plateau (avec MNTrel négatif à 100m)

    // attribue valeur de Z pour chacun des points qui sont sur le lac
    int b(15);
    Pt2di tr(b,b);
    Im2D_INT2 Il2 = LpSurf.image();
    INT2 ** d2 = Il2.data();
    INT nb = Il2.tx();
    INT2 * tx = d2[0];
    INT2 * ty = d2[1];

    std::cout << "attribue valeur d'Altitude à chaque pt des surfaces d'eau\n";

    // MNT trop lent à lire: je copie en mémoire RAM
    Im2D_REAL4 * aMNT=new Im2D_REAL4(sz.x,sz.y);
    Im2D_REAL4 * aMNTrel=new Im2D_REAL4(sz.x,sz.y,0.0);
    ELISE_COPY(mMNT->all_pts(),mMNT->in(),aMNT->out());

    for (INT k=0 ; k<nb ; k++){
        Pt2di pos(tx[k],ty[k]);
        if (mMNTrel.Inside(pos)){
            double minMNT(0);
            // recherche du dans la région de ce point
            ELISE_COPY(rectangle(pos-tr,pos+tr),
                       aMNT->in(700.0),
                       VMin(minMNT)
                       );
            aMNTrel->SetR(pos,minMNT);
        }
    }

    delete aMNT;
    // dilatation - prend 4,5 Gb de RAM avec Dist=15!! sensible le coco
    _dilate(aMNTrel,Surf2Use, double(0.0));


    aMNT=new Im2D_REAL4(sz.x,sz.y);
    ELISE_COPY(mMNT->all_pts(),mMNT->in(),aMNT->out());

    Liste_Pts_INT2 lp(2);
    ELISE_COPY(select(aMNTrel->all_pts(), Surf2Use.in()<seuilDist),
               666,
               lp
               );

    ELISE_COPY(select(lp.all_pts(), aMNT->in_proj()>=aMNTrel->in_proj()),
               aMNT->in_proj()-aMNTrel->in_proj(),
               aMNTrel->oclip()
               );

    ELISE_COPY(select(lp.all_pts(), Surf2Use.in(255)==255), 101.0, aMNTrel->oclip());

    Tiff_Im::CreateFromIm(*aMNTrel,std::string("MNTreltmp.tif"));

    // copie vers mMNTrel       //mDistChanf.in(255)>= Surf2Use.in(255) &&
    ELISE_COPY(select(lp.all_pts(),  mMNTrel.in_proj()>aMNTrel->in_proj()),
               aMNTrel->in_proj(),
               mMNTrel.oclip()
               );
    delete aMNT;
    delete aMNTrel;
}

void c_Appli_FEELU::computeMNTrel(){

    Im2D_REAL4 aMNTdilate(sz.x,sz.y,0.0);

    Im2D_REAL4 * aMNT=new Im2D_REAL4(sz.x,sz.y);
    ELISE_COPY(mMNT->all_pts(),mMNT->in(),aMNT->out());

    // image de synthèse pour distance de chamfer autour d'un pt
    int b(15);
    Pt2di tr(b,b);
    Im2D_U_INT1 myDist(1+2*b,1+2*b,1);
    myDist.SetR(tr,0);
    ChamferNoBorder(&myDist);

    //boucle sur les pt du réseau hydro

    Im2D_INT2 Il2 = lpHydro->image();
    INT2 ** d = Il2.data();
    INT nb = Il2.tx();
    INT2 * tx = d[0];
    INT2 * ty = d[1];

    std::cout << "attribue valeur d'Altitude à chaque pt du réseau hydro\n";
    for (INT k=0 ; k<nb ; k++){
        Pt2di pos(tx[k],ty[k]);
        if (aMNTdilate.Inside(pos)){

            double minMNT(0);
            // recherche du dans la région de ce point, mais avec une préférence pour un pt perpendiculaire au réseau hydro
            ELISE_COPY(select(rectangle(pos-tr,pos+tr), mDistChanf.in(255)==trans(myDist.in(254),-(pos-tr))),
                       aMNT->in(),
                       VMin(minMNT)
                       );
            aMNTdilate.SetR(pos,minMNT);
            // ecriture de ce minimum sur l'axe perpendiculaire au reseau hydro: redondant avec dilate
            /* ELISE_COPY(select(rectangle(pos-tr,pos+tr), mDistChanf.in(255)==trans(myDist.in(254),-(pos-tr)) && mDistChanf.in(255) <seuilDist ),
                       minMNT,
                       aMNTdilate.oclip()
                       );
                       */
        }
    }
    _dilate(&aMNTdilate,mDistChanf,double(0.0),bool(TRUE));
    ELISE_COPY(select(aMNTdilate.all_pts(), mDistChanf.in()<seuilDist &&  aMNT->in()>=aMNTdilate.in()),
               aMNT->in()-aMNTdilate.in(),
               aMNTdilate.out()
               );
    //il reste des valeurs, proche du reseau hydro, qui ont a valeur de l'altitude du ruisseau; la soustraction avec le MNT ne s'est pas faite, je sais pas pk
    ELISE_COPY(select(aMNTdilate.all_pts(), mDistChanf.in()<seuilDist && aMNTdilate.in()>100.0),
               0.0,
               aMNTdilate.out()
               );
    // maintenant que le  dilate est fait je remplace par 100 les valeurs en dehors
    ELISE_COPY(select(aMNTdilate.all_pts(), mDistChanf.in()>=seuilDist),100.0,aMNTdilate.out());
    Tiff_Im::CreateFromIm(aMNTdilate,mNameDelatHOut);
    ELISE_fp::copy_file(mTFWin,NameTFW(mNameDelatHOut),1);
    delete aMNT;
}

// selectionner la valeur de FA associée au pt du ruisseau le plus proche
// mais vu qu'il y a un petit décalage entre le réseau hydro et le MNT/FA, il faut faire en sorte d'aller chercher la valeur max de FA à proximité du réseau hydro
void c_Appli_FEELU::computeFA(){

    // conversion en int
    Im2D_U_INT2 aFAdilate(sz.x,sz.y,0);
    //Im2D_REAL4 aFAdilate(sz.x,sz.y,0);

    // filtre maximum pour rendre l'info plus propre/utilisable
    for (int k(0);k<3;k++){
        ELISE_COPY(mFA.all_pts(),
                   rect_max(mFA.in(0),2),
                   mFA.out()
                   );}

    // image de synthèse pour distance de chamfer autour d'un pt
    int b(15);
    Pt2di tr(b,b);
    Im2D_U_INT1 myDist(1+2*b,1+2*b,1);
    myDist.SetR(tr,0);
    ChamferNoBorder(&myDist);

    //boucle sur les pt du réseau hydro

    Im2D_INT2 Il2 = lpHydro->image();
    INT2 ** d = Il2.data();
    INT nb = Il2.tx();
    INT2 * tx = d[0];
    INT2 * ty = d[1];

    std::cout << "attribue valeur de FA à chaque pt du réseau hydro\n";
    for (INT k=0 ; k<nb ; k++){
        Pt2di pos(tx[k],ty[k]);
        if (mFA.Inside(pos)){

            int maxFA(0);
            // recherche du maximum de FA dans la région de ce point, mais avec une préférence pour un pt perpendiculaire au réseau hydro
            ELISE_COPY(select(rectangle(pos-tr,pos+tr), mDistChanf.in(255)==trans(myDist.in(254),-(pos-tr))),
                       mFA.in(),
                       VMax(maxFA)
                       );
            aFAdilate.SetR(pos,maxFA);
        }
    }


    // ça marche mais les valeurs évoluent très fortement le long du cours d'eau. Il faut rendre tout cela plus propre, mais comment? pour commencer, prendre le max de FA par cours d'eau

    int pct(0);

    for (int i(1) ; i < nb_riv;i++){
        if (mod(i,step)==0){
            pct=pct+2;
            std::cout << pct << "% " << std::endl;usleep(10);
        }

        double aMaxFAriviere(0);
        Liste_Pts_INT2 l3(2);
        ELISE_COPY(select(lpHydro->all_pts(),mHydro.in(0)==i),
                   aFAdilate.in_proj(),
                   VMax(aMaxFAriviere)|l3
                   );
        // on écrit la valeur max pour chacun des cours d'eau
        ELISE_COPY(l3.all_pts(),
                   aMaxFAriviere,
                   aFAdilate.oclip()
                   );
    }

    // dilatation
    _dilate(&aFAdilate,mDistChanf,double(0.0));

    Tiff_Im::CreateFromIm(aFAdilate,mNameFlowAccOut);
    ELISE_fp::copy_file(mTFWin,NameTFW(mNameFlowAccOut),1);

}

//lecture et écriture d'une valeur par rivière dans un fichier texte- en entrée: le FA dilaté
void c_Appli_FEELU::readVal4EachRiver(std::string aOut){

    Im2D_U_INT2 ImFA=Im2D_U_INT2::FromFileStd(mNameFlowAccOut);
    int  pct(0);
    std::map<int,int> aFA;

    for (int i(1) ; i < nb_riv;i++){
        if (mod(i,step)==0){
            pct=pct+2;
            std::cout << pct << "% " << std::endl;usleep(10);
        }
        int aNb(0);
        double aSum(0);
        ELISE_COPY(select(lpHydro->all_pts(),mHydro.in(0)==i),
                   Virgule(ImFA.in(0),1),
                   Virgule(sigma(aSum),sigma(aNb))
                   );
        // on écrit la valeur moy pour chacun des cours d'eau
        int aVal=(aSum/(aNb*1000));
        if (aVal<0) {aVal=0;}
        aFA.emplace(std::make_pair(i,aVal));
    }
    // écriture du vecteur dans un fichier texte
    std::ofstream aFile(aOut.c_str());
    aFile.precision(10);
    aFile << "ID;FA\n";
    for (auto & pair : aFA){

        aFile << pair.first << ";" << pair.second << "\n";
    }
    aFile.close();
}


void c_Appli_FEELU::computeDistMap1by1(Im2D_U_INT1 *Ibin, Im2D_U_INT1 * aOut){

    U_INT1 ** d = Ibin->data();
    int count(0); // counter
    Neighbourhood V8=Neighbourhood::v8();

    for (INT x=0; x < Ibin->sz().x; x++)
    {
        for (INT y=0; y < Ibin->sz().y; y++)
        {
            if (d[y][x] ==1)
            {
                count++;

                std::cout <<"-";usleep(1);
                Liste_Pts_INT2 Lp(2);
                Pt2di pmax,pmin;
                ELISE_COPY
                        (
                            conc
                            (
                                Pt2di(x,y),
                                Ibin->neigh_test_and_set(V8, 1, 0,20) ),
                            666,
                            Lp);
                ELISE_COPY(Lp.all_pts(),Virgule(FX,FY),((pmax.VMax())|(pmin.VMin())));
                // ici je met les critères pour déterminer la distance max

                int DM(0), surf(Lp.card()/6);


                if( surf<100){DM=10;}else{
                    if (surf<1000){DM=30;} else{
                        DM=seuilDist;}
                }

                add2DistMap(&Lp,pmin,pmax, aOut,DM);

            }
        }
    }
    std::cout << "\n";
}


void c_Appli_FEELU::add2DistMap(Liste_Pts_INT2 * lp, Pt2di pmin,Pt2di pmax, Im2D_U_INT1 * aOut, int DistMax){

    // applique un buffer autour de la rivière
    //std::cout << "avant buffer " << pmin << " " << pmax << std::endl;
    //c++;

    pmin += Pt2di(-DistMax,-DistMax);
    pmax += Pt2di(DistMax,DistMax);

    // cree une image
    Im2D_U_INT1 aDist(pmax.x-pmin.x,pmax.y-pmin.y,1);

    // copier le flux dedans
    Im2D_INT2 Il1 = lp->image();
    INT2 ** d = Il1.data();
    INT nb = Il1.tx();
    INT2 * tx = d[0];
    INT2 * ty = d[1];
    for (INT k=0 ; k<nb ; k++){
        Pt2di pos(tx[k]-pmin.x,ty[k]-pmin.y);
        aDist.SetI(pos,0);
    }

    //calcul dist de chamfer
    ChamferNoBorder(&aDist);
    // modif valeur de distance si au dela du seuil autorisé
    ELISE_COPY(select(aDist.all_pts(),aDist.in()>DistMax),
               255,
               aDist.out());


    //if (c==40){ Tiff_Im::CreateFromIm(aDist,std::string("Dist1by1A.tif"));}
    // ajout dans mapAllDist
    //std::cout << " pmin " << pmin << " pmax " << pmax << std::endl;
    ELISE_COPY(select(rectangle(pmin,pmax), trans(aDist.in(),-pmin)<aOut->in_proj())
               ,trans(aDist.in(),-pmin)
               ,aOut->oclip()// clip car le buffer peut faire sortir de l'image DistAll
               );
}


int main_test2(int argc,char ** argv)
{
    c_Appli_FEELU(argc,argv);
    return EXIT_SUCCESS;
}


// Chamfer::d32.im_dist compute chamfer distance from border of the image, i do not like that at all. this function add a border prior to compute chanfer distance and remove it afterward
void c_Appli_FEELU::ChamferNoBorder(Im2D<U_INT1,INT> * i2d)
{
    int border(200);
    Im2D_U_INT1 tmp(i2d->sz().x+2*border,i2d->sz().y+2*border,1);
    ELISE_COPY(select(tmp.all_pts(),trans(i2d->in(1),-Pt2di(border,border))==0),0,tmp.out());
    Chamfer::d32.im_dist(tmp);
    ELISE_COPY(i2d->all_pts(),trans(tmp.in(255),Pt2di(border,border)),i2d->oclip());
}

template <class T,class TB>
void c_Appli_FEELU::_dilate(Im2D<T,TB> * aIm2Dilate, Im2D_U_INT1 & aImDist, double aND, bool aMoy){

    std::cout << "DILATATION\n";
    TB ND=aND;
    Liste_Pts_INT2 lpSeuilDist(2);
    ELISE_COPY(select(aImDist.all_pts(), aImDist.in()<seuilDist), 666, lpSeuilDist );
    for (int distCh(1);distCh <seuilDist;distCh++)
    {
        std::cout << "Distance " << distCh << std::endl;
        // sauve les pt dans une listes
        Liste_Pts_INT2 l1(2);
        ELISE_COPY(select(lpSeuilDist.all_pts(), aImDist.in(255)==distCh), 666, l1);
        //ELISE_COPY(select(aImDist.all_pts(), aImDist.in(255)==distCh), 666, l1);
        Im2D_INT2 Il1 = l1.image();
        INT2 ** d = Il1.data();
        INT nb = Il1.tx();
        INT2 * tx = d[0];
        INT2 * ty = d[1];
        for (INT k=0 ; k<nb ; k++){
            Pt2di pos(tx[k],ty[k]);
            if (aIm2Dilate->Inside(pos)){

                TB aV1(aIm2Dilate->GetR(pos+Pt2di(0,1))),
                        aV2(aIm2Dilate->GetR(pos+Pt2di(0,-1))),
                        aV3(aIm2Dilate->GetR(pos+Pt2di(1,0))),
                        aV4(aIm2Dilate->GetR(pos+Pt2di(-1,0))),
                        aV5(aIm2Dilate->GetR(pos+Pt2di(1,1))),
                        aV6(aIm2Dilate->GetR(pos+Pt2di(1,-1))),
                        aV7(aIm2Dilate->GetR(pos+Pt2di(-1,1))),
                        aV8(aIm2Dilate->GetR(pos+Pt2di(-1,-1))),
                        mean(0),
                        n(0);
                // si pas vérif, prend en compte les valeurs de pt qui viennent de passer dans la boucle ci-présente
                if (aV1!=ND && aImDist.GetR(pos+Pt2di(0,1))<distCh) {n++; } else { aV1=0;}
                if (aV2!=ND && aImDist.GetR(pos+Pt2di(0,-1))<distCh) {n++ ;} else { aV2=0;}
                if (aV3!=ND && aImDist.GetR(pos+Pt2di(1,0))<distCh) {n++ ;} else { aV3=0;}
                if (aV4!=ND && aImDist.GetR(pos+Pt2di(-1,0))<distCh) {n++; } else { aV4=0;}
                if (aV5!=ND && aImDist.GetR(pos+Pt2di(1,1))<distCh) {n++ ;} else { aV5=0;}
                if (aV6!=ND && aImDist.GetR(pos+Pt2di(1,-1))<distCh) {n++ ;} else { aV6=0;}
                if (aV7!=ND && aImDist.GetR(pos+Pt2di(-1,1))<distCh) {n++ ;} else { aV7=0;}
                if (aV8!=ND && aImDist.GetR(pos+Pt2di(-1,-1))<distCh) {n++ ;} else { aV8=0;}
                if (n!=0) mean=(aV1+aV2+aV3+aV4+aV5+aV6+aV7+aV8)/n;
                aIm2Dilate->SetI(pos,mean);
            }
        }
        // lissage
        if (aMoy && distCh>5){
            for (int k(0);k<3;k++){
                for (INT k=0 ; k<nb ; k++){
                    Pt2di pos(tx[k],ty[k]);
                    if (aIm2Dilate->Inside(pos)){

                        TB aV0(aIm2Dilate->GetR(pos)),
                                aV1(aIm2Dilate->GetR(pos+Pt2di(0,1))),
                                aV2(aIm2Dilate->GetR(pos+Pt2di(0,-1))),
                                aV3(aIm2Dilate->GetR(pos+Pt2di(1,0))),
                                aV4(aIm2Dilate->GetR(pos+Pt2di(-1,0))),
                                aV5(aIm2Dilate->GetR(pos+Pt2di(1,1))),
                                aV6(aIm2Dilate->GetR(pos+Pt2di(1,-1))),
                                aV7(aIm2Dilate->GetR(pos+Pt2di(-1,1))),
                                aV8(aIm2Dilate->GetR(pos+Pt2di(-1,-1))),
                                mean(0),
                                n(1);// la pos 0,0
                        if (aV1!=ND && aImDist.GetR(pos+Pt2di(0,1))==distCh) {n++; } else { aV1=0;}
                        if (aV2!=ND && aImDist.GetR(pos+Pt2di(0,-1))==distCh) {n++ ;} else { aV2=0;}
                        if (aV3!=ND && aImDist.GetR(pos+Pt2di(1,0))==distCh) {n++ ;} else { aV3=0;}
                        if (aV4!=ND && aImDist.GetR(pos+Pt2di(-1,0))==distCh) {n++; } else { aV4=0;}
                        if (aV5!=ND && aImDist.GetR(pos+Pt2di(1,1))==distCh) {n++ ;} else { aV5=0;}
                        if (aV6!=ND && aImDist.GetR(pos+Pt2di(1,-1))==distCh) {n++ ;} else { aV6=0;}
                        if (aV7!=ND && aImDist.GetR(pos+Pt2di(-1,1))==distCh) {n++ ;} else { aV7=0;}
                        if (aV8!=ND && aImDist.GetR(pos+Pt2di(-1,-1))==distCh) {n++ ;} else { aV8=0;}
                        if (n!=0) mean=(aV0+aV1+aV2+aV3+aV4+aV5+aV6+aV7+aV8)/n;
                        aIm2Dilate->SetI(pos,mean);
                    }
                }
            }

        }
    }
}


std::map<int,std::vector<std::string>> parseCSV(std::string aFileIn, char aDelim){
    qi::rule<std::string::const_iterator, std::string()> quoted_string = '"' >> *(qi::char_ - '"') >> '"';
    qi::rule<std::string::const_iterator, std::string()> valid_characters = qi::char_ - '"' - aDelim;
    qi::rule<std::string::const_iterator, std::string()> item = *(quoted_string | valid_characters );
    qi::rule<std::string::const_iterator, std::vector<std::string>()> csv_parser = item % aDelim;
    std::vector<std::string> result;
    std::map<int,std::vector<std::string>> aOut;
    std::ifstream aFichier(aFileIn.c_str());
    if(aFichier)
    {
        std::string aLine;
        // premiere ligne, entete
        getline(aFichier,aLine,'\n');
        // définir la colonne de l'ID
        std::string::const_iterator s_begin = aLine.begin();
        std::string::const_iterator s_end = aLine.end();
        bool r = boost::spirit::qi::parse(s_begin, s_end, csv_parser, result);
        assert(r == true);
        assert(s_begin == s_end);
        // cherche le num de la colonne ID
        int j(0);
        bool found(0);
        for (std::vector<std::string>::iterator it = result.begin() ; it != result.end(); ++it){
            if ((*it=="ID") | (*it=="ID_1") ){
                found=1;
                break;}
            j++;

        }

        if (found==0){
            std::cout << "Warn, in file " << aFileIn << " with separator " << aDelim << ", colonne ID not found \n";
        }else{

            while(!aFichier.eof())
            {
                getline(aFichier,aLine,'\n');
                if(aLine.size() != 0)
                {
                    std::string::const_iterator s_begin2 = aLine.begin();
                    std::string::const_iterator s_end2 = aLine.end();
                    std::vector<std::string> result2;
                    bool r = boost::spirit::qi::parse(s_begin2, s_end2, csv_parser, result2);
                    assert(r == true);
                    assert(s_begin == s_end);
                    // ajout d'un element à la map
                    // attention, ligne vide dans un fichier. faut gerer l'eventuallité
                    char* ptr;
                    strtol(result2.at(j).c_str(), &ptr, 10);

                    if (*ptr == '\0'){aOut.emplace(std::stoi(result2.at(j)),result2);}
                }
            }

        }
        aFichier.close();
    } else {
        std::cout << "file " << aFileIn << " not found " <<std::endl;
    }
    return aOut;
};

std::string NameTFW(std::string aOrtName)
{
    std::string TFWName=aOrtName.substr(0, aOrtName.size()-3)+"tfw";
    return TFWName;
}

