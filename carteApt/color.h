#ifndef COLOR_H
#define COLOR_H



class color
{
public:
    ~color(){}
    color(int R,int G,int B,std::string name="toto"):mR(R),mG(G),mB(B),mStyleClassName(name){isDark();}
    color(std::string aHex,std::string name):mStyleClassName(name){
        // j'enlève le diaise qui semble ne pas convenir
        const char* c=aHex.substr(1,aHex.size()).c_str();
        sscanf(c, "%02x%02x%02x", &mR, &mG, &mB);
        //std::cout << std::to_string(mR) << ";" <<std::to_string(mG) << ";" <<std::to_string(mB) << std::endl;
        isDark();
    }
    color(std::string aHex){
        const char* c=aHex.substr(1,aHex.size()).c_str();
        // j'enlève le diaise qui semble ne pas convenir pour le nom de style il faut également s'assurer que le code ne commence pas par un numéro.
        mStyleClassName=aHex.substr(1,aHex.size());
        if (isdigit(mStyleClassName[0])){ mStyleClassName="a"+mStyleClassName;}
        sscanf(c, "%02x%02x%02x", &mR, &mG, &mB);
        //std::cout << std::to_string(mR) << ";" <<std::to_string(mG) << ";" <<std::to_string(mB) << std::endl;
        isDark();
    }
    int mR,mG,mB;
    void set(int &R,int &G,int &B){
        R=mR;
        G=mG;
        B=mB;
        isDark();
    }

    void isDark(){
       double hsp = 0.299 * pow(mR,2) + 0.587 * pow(mG,2) + 0.114 * pow(mB,2);
        //if (hsp<127.5) {mDark=true;} else {mDark=false;}
        if (hsp<210) {mDark=true;} else {mDark=false;}
    }

    bool dark(){return mDark;}

    std::string cat(){ return " R:" + std::to_string(mR)+", G:"+std::to_string(mG)+", B"+std::to_string(mB);}
    std::string cat2(){ return std::to_string(mR)+" "+std::to_string(mG)+" "+std::to_string(mB);}
    std::string catHex(){
        unsigned long hex= ((mR & 0xff) << 16) + ((mG & 0xff) << 8) + (mB & 0xff);
        return "#"+std::to_string(hex);
    }
    std::string getStyleName(){return "."+mStyleClassName;}
    std::string getStyleNameShort(){return mStyleClassName;}
    std::string mStyleClassName;
    bool mDark;
};



#endif // COLOR_H
