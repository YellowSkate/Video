#include "util.hpp"
#include <typeinfo>

int JsonTest(){
    struct stu{
        std::string name;
        int age;
        int score[3];
    };
    stu Andy ={"Andy",18,{99,91,88}};
    Json::Value val;
    val["name"]=Andy.name;
    val["age"]=Andy.age;
    for(int i=0;i<sizeof(Andy.score)/sizeof(int);i++){
        val["score"].append(Andy.score[i]); 
    }
    std::string* s=new std::string();
    if(video::JsonUtil::Serialize(val,s)){
        cout<<*s<<endl;
    }
    cout<<"========UnSerialize() test========\n";

    Json::Value temp;
    if(video::JsonUtil::UnSerialize(*s,&temp)){
        cout<<typeid(temp).name()<<"   "<<temp["age"]<<endl;
        cout<<typeid(temp["name"]).name()<<"   "<<temp["name"]<<endl;
        cout<<typeid(temp["score"]).name()<<"   "<<temp["score"].size()<<endl;
        cout<<typeid(temp["score"][0]).name()<<"   "<<temp["score"][0].size()<<endl;
    }
    delete s;
    return 0;
}
void mysqltest(){
    Json::Value val;
    // val["name"]="明朝那些事8";
    // val["category"]="历史";
    // val["content"]="123123";
    // val["video_url"]="lllllll";
    // val["image_url"]="555555555555";
    video::TbVideo tb;
    // if(tb.Insert(val)){
    //     cout<<"success"<<endl;
    // }
    // cout<<"===============================\n";
    
    Json::Value val2;
    val2["name"]="大明王朝";
    val2["category"]="历史";
    val2["content"]="一部历史神剧";
    val2["video_url"]="lllllll";
    val2["image_url"]="555555555555";
    
    if(tb.Update(13,val2)){
        cout<<"success"<<endl;
    }
    cout<<"===============================\n";
    
    // if(tb.Delete(18)){  
    //     cout<<"success"<<endl;
    // }
    cout<<"===============================\n";

    Json::Value v;
    if(tb.SelectAll(&v)){
        std::string s;
        video::JsonUtil::Serialize(v,&s);
        cout<<"success"<<endl;

        cout<<s<<endl;
    }
    cout<<"===============================\n";

    Json::Value v2;

    if(tb.SelectID(13,&v2)){
        std::string s;
        video::JsonUtil::Serialize(v2,&s);
        cout<<"success"<<endl;

        cout<<s<<endl;
    }
    cout<<"===============================\n";

    Json::Value v3;

    if(tb.SelectCategory("科幻",&v3)){
        std::string s;
        video::JsonUtil::Serialize(v3,&s);
        cout<<"success"<<endl;

        cout<<s<<endl;
    }
    cout<<"===============================\n";

    Json::Value v4;

    if(tb.SelectLike("那些",&v4)){
        std::string s;
        video::JsonUtil::Serialize(v4,&s);
        cout<<"success"<<endl;

        cout<<s<<endl;
    }

    cout<<"===============================\n";
    return ;
}

void FileTest(){
    video::FileUtil("./www").CreateDirectory();
    video::FileUtil("./www/index.html").WriteContent("[***************Hello world Util**********]\n");

    cout<<"文件大小 : "<<video::FileUtil("./www/index.html").FileSize()<<endl;
    std::string body;
    video::FileUtil("./www/index.html").ReadContent(&body);
    cout<<body<<endl;

    cout<<"6666"<<endl;

    return;
}




int main(int argc,char* argv[]){
    int port=atoi(argv[1]);
    video::Server server(port);

    server.RunModule();
    
    return 0;
}