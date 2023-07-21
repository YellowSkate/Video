#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>
#include <cstdlib>

#include <mutex>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <jsoncpp/json/json.h>
#include <mysql/mysql.h>

#include "httplib.h"

using std::cerr;
using std::cout;
using std::endl;

namespace video
{
    class JsonUtil
    {
    public:
        static bool Serialize(const Json::Value &value, std::string *str)
        {
            Json::StreamWriterBuilder swb; // 相对于之前的bosst搜索引擎项目 ,这是json高版本更推荐的写法
            std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());

            std::stringstream ss;
            int ret = sw->write(value, &ss);
            if (ret != 0)
            {
                cerr << "Serialiae failed\n";
                return false;
            }
            *str = ss.str();
            return true;
        }
        static bool UnSerialize(const std::string &str, Json::Value *value)
        {
            Json::CharReaderBuilder crb;
            std::unique_ptr<Json::CharReader> cr(crb.newCharReader());

            std::string err;
            bool ret = cr->parse(str.c_str(), str.c_str() + str.size(), value, &err);
            if (ret == false)
            {
                cerr << "UnSerialize failed\n";
            }
            return true;
        }
    };

// mysql 模块
#define HOST "127.0.0.1"
#define USER "video"
#define PASS "123456"
#define DATABASE "videoer"

    static MYSQL *MysqlInit()
    { // 数据库的初始化
        MYSQL *mysql = mysql_init(nullptr);
        if (nullptr == mysql)
        {
            cerr << "init mysql instance failed\n";
            return nullptr;
        }
        if (nullptr == mysql_real_connect(mysql, HOST, USER, PASS, DATABASE, 0, nullptr, 0))
        {
            cerr << "connect mysql server failed\n";
            mysql_close(mysql);
        }
        mysql_set_character_set(mysql, "utf8");
        return mysql;
    }
    static void MysqlDestroy(MYSQL *mysql)
    { // 数据库的销毁
        if (nullptr == mysql)
            return;
        mysql_close(mysql);
    }

    static bool MysqlQuery(MYSQL *mysql, const std::string &sql)
    { // 数据库的请求

        // 保证一次只执行一个语句
        //  const std::string sql=query.substr(0,query.find(';'));

        cout << "执行: " << sql << endl;
        int ret = mysql_query(mysql, sql.c_str());
        if (ret != 0)
        {
            cerr << sql << endl;
            cerr << mysql_errno << endl;
            return false;
        }
        return true;
    }
    class TbVideo
    {
    private:
        MYSQL *_mysql;     // 一个对象就是一个客户端，管理一张表
        std::mutex _mutex; // 防备操作对象在多线程中使用存在的线程安全 问题
    public:
        TbVideo()
        {
            _mysql = MysqlInit();
            if (_mysql == nullptr)
            {
                exit(-1);
            }
        } // 完成mysql句柄初始化
        ~TbVideo()
        {
            MysqlDestroy(_mysql);
        } // 释放msyql操作句柄
        bool Insert(const Json::Value &video)
        { // 新增-传入视频信息
            std::string sql = "insert tb_video values(null";
            sql += ",";
            sql += "'";
            sql += video["name"].asCString();
            sql += "'";
            sql += ",";
            sql += "'";
            sql += video["category"].asCString();
            sql += "'";
            sql += ",";
            sql += "'";
            sql += video["content"].asCString();
            sql += "'";
            sql += ",";
            sql += "'";
            sql += video["video_url"].asCString();
            sql += "'";
            sql += ",";
            sql += "'";
            sql += video["image_url"].asCString();
            sql += "'";
            sql += ");";
            return MysqlQuery(_mysql, sql);
        }
        bool Update(int video_id, const Json::Value &video)
        { // 修改-传入视频id，和信息
            std::string sql = "update tb_video set ";
            sql += " name=";
            sql += "'";
            sql += video["name"].asCString();
            sql += "'";
            sql += " ,category=";
            sql += "'";
            sql += video["category"].asCString();
            sql += "'";
            sql += " ,content=";
            sql += "'";
            sql += video["content"].asCString();
            sql += "'";
            sql += " ,video_url=";
            sql += "'";
            sql += video["video_url"].asCString();
            sql += "'";
            sql += " ,image_url=";
            sql += "'";
            sql += video["image_url"].asCString();
            sql += "'";
            sql += "where id=";
            sql += std::to_string(video_id);

            sql += ";";

            return MysqlQuery(_mysql, sql);
        }
        bool Delete(const int video_id)
        { // 删除-传入视频ID
            std::string sql = "delete from tb_video where id=";
            sql += std::to_string(video_id);
            sql += ";";
            return MysqlQuery(_mysql, sql);
        }
        bool DeleteName(const std::string &name)
        { // 内部调用 ,根据名字删除视频
            std::string sql = "delete from tb_video where name = '";
            sql += name;
            sql += "' ;";
            return MysqlQuery(_mysql, sql);
        }

        // 因为select 需要返回结果集 ,故记得上锁保护
        bool SelectAll(Json::Value *videos)
        { // 查询所有--输出所有视频信息
            std::string sql = "select * from tb_video;";

            _mutex.lock();
            bool ret = MysqlQuery(_mysql, sql);
            if (ret == false)
            {
                _mutex.unlock();
                return false;
            }
            MYSQL_RES *res = mysql_store_result(_mysql); // 获取结果集
            _mutex.unlock();

            if (nullptr == res)
            {
                cerr << "mysql_store_result failed\n";
                return false;
            }
            int num_rows = mysql_num_rows(res);
            for (int i = 0; i < num_rows; i++)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                Json::Value val;
                val["id"] = atoi(row[0]);
                val["name"] = row[1];
                val["category"] = row[2];
                val["content"] = row[3];
                val["video_url"] = row[4];
                val["image_url"] = row[5];
                videos->append(val); // 添加结果
            }
            mysql_free_result(res); // 释放结果集
            return true;
        }
        bool SelectID(int video_id, Json::Value *video)
        { // 查询单个-输入视频id,输出信息
            std::string sql = "select * from tb_video where id=";
            sql += std::to_string(video_id);
            sql += ";";

            _mutex.lock();
            bool ret = MysqlQuery(_mysql, sql);
            if (ret == false)
            {
                _mutex.unlock();
                return false;
            }
            MYSQL_RES *res = mysql_store_result(_mysql); // 获取结果集
            _mutex.unlock();

            if (nullptr == res)
            {
                cerr << "mysql_store_result failed\n";
                return false;
            }

            MYSQL_ROW row = mysql_fetch_row(res);

            (*video)["id"] = video_id;
            (*video)["name"] = row[1];
            (*video)["category"] = row[2];
            (*video)["content"] = row[3];
            (*video)["video_url"] = row[4];
            (*video)["image_url"] = row[5];

            mysql_free_result(res); // 释放结果集
            return true;
        }
        bool SelectCategory(const std::string &key, Json::Value *videos)
        { // 分类匹配-输入名称关键字，输出视频信息
            std::string sql = "select * from tb_video where Category='";
            sql += key;

            sql += "' ;";
            _mutex.lock();
            bool ret = MysqlQuery(_mysql, sql);
            if (ret == false)
            {
                _mutex.unlock();
                return false;
            }
            MYSQL_RES *res = mysql_store_result(_mysql); // 获取结果集
            _mutex.unlock();

            if (nullptr == res)
            {
                cerr << "mysql_store_result failed\n";
                return false;
            }
            int num_rows = mysql_num_rows(res);
            for (int i = 0; i < num_rows; i++)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                Json::Value val;
                val["id"] = atoi(row[0]);
                val["name"] = row[1];
                val["category"] = row[2];
                val["content"] = row[3];
                val["video_url"] = row[4];
                val["image_url"] = row[5];
                videos->append(val); // 添加结果
            }
            mysql_free_result(res); // 释放结果集
            return true;
        }
        bool SelectLike(const std::string &key, Json::Value *videos)
        { // 模糊匹配-输入名称关键字，输出视频信息
            std::string sql = "select * from tb_video where name like '%";
            sql += key; // 模糊匹配
            sql += "%';";
            _mutex.lock();
            bool ret = MysqlQuery(_mysql, sql);
            if (ret == false)
            {
                _mutex.unlock();
                return false;
            }
            MYSQL_RES *res = mysql_store_result(_mysql); // 获取结果集
            _mutex.unlock();

            if (nullptr == res)
            {
                cerr << "mysql_store_result failed\n";
                return false;
            }
            int num_rows = mysql_num_rows(res);
            for (int i = 0; i < num_rows; i++)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                Json::Value val;
                val["id"] = atoi(row[0]);
                val["name"] = row[1];
                val["category"] = row[2];
                val["content"] = row[3];
                val["video_url"] = row[4];
                val["image_url"] = row[5];
                videos->append(val); // 添加结果
            }
            mysql_free_result(res); // 释放结果集
            return true;
        }

        bool SelectDouble(const std::string &category, const std::string &key, Json::Value *videos)
        {
            std::string sql = "select * from tb_video where Category='";
            sql += category;
            sql += "' "; // 先分类
            sql += " and name like '%";
            sql += key; // 模糊匹配
            sql += "%';";
            _mutex.lock();
            bool ret = MysqlQuery(_mysql, sql);
            if (ret == false)
            {
                _mutex.unlock();
                return false;
            }
            MYSQL_RES *res = mysql_store_result(_mysql); // 获取结果集
            _mutex.unlock();

            if (nullptr == res)
            {
                cerr << "mysql_store_result failed\n";
                return false;
            }
            int num_rows = mysql_num_rows(res);
            for (int i = 0; i < num_rows; i++)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                Json::Value val;
                val["id"] = atoi(row[0]);
                val["name"] = row[1];
                val["category"] = row[2];
                val["content"] = row[3];
                val["video_url"] = row[4];
                val["image_url"] = row[5];
                videos->append(val); // 添加结果
            }
            mysql_free_result(res); // 释放结果集
            return true;
        }
    };

    class FileUtil
    {
    private:
        std::string _name; // 路径名称

    public:
        FileUtil(const std::string &name) : _name(name){};
        bool Exists()
        {                                          // 判断文件是否存在
            int ret = access(_name.c_str(), F_OK); // F_OK 检查文件是否存在 ,存在返回 0;
            if (0 != ret)
            {
                return false;
            }
            return true;
        }
        size_t FileSize()
        {
            if (this->Exists() == false)
            {
                return 0;
            }
            struct stat st;                     // stat 用于获取文件属性的接口;  成员st_size 即文件大小
            int ret = stat(_name.c_str(), &st); // 系统调用接口
            if (ret != 0)
                return 0; // 获取失败

            return st.st_size;
        }
        bool ReadContent(std::string *content)
        {
            std::ifstream ifs;
            ifs.open(_name, std::ios::binary);
            if (ifs.is_open() == false)
            {
                std::cerr << "open file failed!\n";
                return false;
            }
            size_t flen = this->FileSize();
            content->resize(flen);

            ifs.read((char *)&((*content)[0]), flen); // c_str() 返回的是const类型....

            if (ifs.good() == false)
            {

                std::cerr << "read file content failed\n";
                ifs.close();
                return false;
            }

            ifs.close();

            return true;
        }
        bool WriteContent(const std::string content)
        {
            std::ofstream ofs;
            ofs.open(_name, std::ios::binary);
            if (false == ofs.is_open())
            {
                std::cerr << "open file failed!\n";
                return false;
            }

            ofs.write(content.c_str(), content.size());
            if (false == ofs.good())
            {
                std::cerr << "write file failed!\n";
                ofs.close();
                return false;
            }
            ofs.close();
            return true;
        }
        bool CreateDirectory()
        {
            if (this->Exists())
            {
                return true;
            }
            if (0 != mkdir(_name.c_str(), 0777))
            {
                std::cerr << "mkdir failed!\n";

                return false;
            }
            return true;
        }
    };

// http 模块
#define WWW_ROOT "./www"
#define VIDEO_ROOT "/video/"
#define IMAGE_ROOT "/image/"
    // 因为httplib基于多线程，因此数据管理对象需要在多线程中访问，为了便于访问定义全局变量
    TbVideo *tb_video = NULL;

    class Server
    {
    private:
        int _port;            // 服务器的 监听端口
        httplib::Server _srv; // 用于搭建http服务器
    private:
        // 对应的业务处理接口
        static void Insert(const httplib::Request &req, httplib::Response &rsp)
        {

            cout << "DEBUG: Insert start" << endl;

            // if (req.has_file("name") == false ||
            //     req.has_file("category") == false ||
            //     req.has_file("conten") == false ||
            //     req.has_file("video_url") == false ||
            //     req.has_file("image_url") == false){

            //     rsp.status = 400;
            //     rsp.body = R"({"result":false, "reason":"上传信息错误"})";
            //     rsp.set_header("Content-Type", "application/json");
            //     return;
            // }

            httplib::MultipartFormData name = req.get_file_value("name"); // 视频名称
            httplib::MultipartFormData category = req.get_file_value("category");
            httplib::MultipartFormData req_content = req.get_file_value("content");
            httplib::MultipartFormData video_url = req.get_file_value("video_url");
            httplib::MultipartFormData image_url = req.get_file_value("image_url");

            std::string video_name = name.content;
            std::string video_category = category.content;
            std::string video_content = req_content.content;

            // 插入数据库
            Json::Value video_json;

            video_json["name"] = video_name;
            video_json["category"] = video_category;
            video_json["content"] = video_content;
            video_json["video_url"] = std::string(VIDEO_ROOT) + video_name + video_url.filename;
            video_json["image_url"] = std::string(IMAGE_ROOT) + video_name + image_url.filename;

            if (tb_video->Insert(video_json) == false)
            {
                rsp.status = 500;
                rsp.body = R"({"result":false, "reason":"数据库插入失败"})";
                rsp.set_header("Content-Type", "application/json");
                return;
            }

            // 设置存储路径
            std::string video_path = std::string(WWW_ROOT) + std::string(VIDEO_ROOT) + video_name + video_url.filename;
            std::string image_path = std::string(WWW_ROOT) + std::string(IMAGE_ROOT) + video_name + image_url.filename;

            // 写入存储路径
            if (FileUtil(video_path).WriteContent(video_url.content) == false)
            {
                tb_video->DeleteName(video_name); // 删除原数据
                rsp.status = 500;
                rsp.body = R"({"result":false, "reason":"视频文件存储失败"})";
                rsp.set_header("Content-Type", "application/json");
                return;
            }

            if (FileUtil(image_path).WriteContent(image_url.content) == false)
            {
                tb_video->DeleteName(video_name); // 删除原数据
                rsp.status = 500;
                rsp.body = R"({"result":false, "reason":"图片文件存储失败"})";
                rsp.set_header("Content-Type", "application/json");
                return;
            }
            rsp.set_redirect("/index.html", 303);
            cout << "DEBUG: Insert complete!!!" << endl;
            return;
        }
        static void Update(const httplib::Request &req, httplib::Response &rsp)
        {

            // 获取需要修改的视频id
            int video_id = atoi(req.matches[1].str().c_str());

            Json::Value v;
            if (JsonUtil::UnSerialize(req.body, &v) == false)
            {
                rsp.status = 400;
                rsp.body = R"({"result":false, "reason":"解析视频失败"})";
                rsp.set_header("Content-Type", "application/json");
                return;
            }

            // 修改
            if (tb_video->Update(video_id, v) == false)
            {
                rsp.status = 500;
                rsp.body = R"({"result":false, "reason":"修改数据库信息失败"})";
                rsp.set_header("Content-Type", "application/json");
                return;
            }
            return;
        }
        static void Delete(const httplib::Request &req, httplib::Response &rsp)
        {

            // 获取id
            int video_id = atoi(req.matches[1].str().c_str());
            cout << "debug :: delete " << video_id << " start " << endl;

            // 删除相关文件
            Json::Value v;
            if (tb_video->SelectID(video_id, &v) == false)
            {
                rsp.status = 500;
                rsp.body = R"({"result":false, "reason":"视频不存在"})";
                rsp.set_header("Content-Type", "application/json");
                return;
            }

            std::string video_path = WWW_ROOT + v["video_url"].asString();
            cout << "rm " << video_path << endl;
            std::string image_path = WWW_ROOT + v["image_url"].asString();
            cout << "rm " << image_path << endl;

            remove(video_path.c_str());
            remove(image_path.c_str());

            // 删除数据库信息

            if (tb_video->Delete(video_id) == false)
            {
                rsp.status = 500;
                rsp.body = R"({"result":false, "reason":"数据库删除失败"})";
                rsp.set_header("Content-Type", "application/json");
                return;
            }
            cout << "debug :: delete end" << endl;
            return;
        }
        static void SelectOne(const httplib::Request &req, httplib::Response &rsp)
        {
            // 获取id

            int video_id = atoi(req.matches[1].str().c_str());

            // 查找相关数据库信息
            Json::Value v;
            if (tb_video->SelectID(video_id, &v) == false)
            {
                rsp.status = 500;
                rsp.body = R"({"result":false, "reason":"视频不存在"})";
                rsp.set_header("Content-Type", "application/json");
                return;
            }
            
            JsonUtil::Serialize(v, &rsp.body);
            rsp.set_header("Content-Type", "application/json");

            return;
        }
        static void Select(const httplib::Request &req, httplib::Response &rsp)
        {
            // /vide  ||  /video?search="关键字"
            bool like_flag = false;     // 默认没有模糊查找
            bool category_flag = false; // 默认没有分类
            std::string search_key;
            std::string category_key;
            if (req.has_param("search") == true)
            {
                like_flag = true; // 模糊匹配
                search_key = req.get_param_value("search");
            }
            if (req.has_param("category") == true)
            {
                category_flag = true; // 开启分类搜搜
                category_key = req.get_param_value("category");
            }
            Json::Value v;
            if (like_flag == false && category_flag == false)
            { // 查询所有
                if (tb_video->SelectAll(&v) == false)
                {
                    rsp.status = 500;
                    rsp.body = R"({"result":false, "reason":"视频不存在"})";
                    rsp.set_header("Content-Type", "application/json");
                    return;
                }
            }
            else
            { // 特殊查找  category
                if (like_flag == true && category_flag == true)
                { // 判断是不是 复合查找
                    if (tb_video->SelectDouble(category_key, search_key, &v) == false)
                    {
                        rsp.status = 500;
                        rsp.body = R"({"result":false, "reason":"多重匹配视频不存在"})";
                        rsp.set_header("Content-Type", "application/json");
                        return;
                    }
                }
                else if (category_flag == true)
                {
                    if (tb_video->SelectCategory(category_key, &v) == false)
                    {
                        rsp.status = 500;
                        rsp.body = R"({"result":false, "reason":"多重匹配视频不存在"})";
                        rsp.set_header("Content-Type", "application/json");
                        return;
                    }
                }
                else if (tb_video->SelectLike(search_key, &v) == false)
                {
                    rsp.status = 500;
                    rsp.body = R"({"result":false, "reason":"模糊匹配视频不存在"})";
                    rsp.set_header("Content-Type", "application/json");
                    return;
                }
                else
                {

                    ;
                }
            }
            cout << "DEBUG:: C: " + category_key + "  S: " + search_key << endl;
            JsonUtil::Serialize(v, &rsp.body);
            rsp.set_header("Content-Type", "application/json");

            return;
        }
        static void SelectCategory(const httplib::Request &req, httplib::Response &rsp)
        {
        }

    public:
        Server(int port) : _port(port) {}
        bool RunModule()
        { // 建立请求与处理函数的映射关系，设置静态资源根目录，启动服务器，
            // 初始化数据库管理模块 ,建立指定目录

            tb_video = new TbVideo();

            FileUtil(WWW_ROOT).CreateDirectory();
            std::string video_path = std::string(WWW_ROOT) + std::string(VIDEO_ROOT);
            FileUtil(video_path).CreateDirectory();
            std::string image_path = std::string(WWW_ROOT) + std::string(IMAGE_ROOT);
            FileUtil(image_path).CreateDirectory();

            // 搭建http 服务
            // 1. 设置静态资源根目录
            _srv.set_mount_point("/", WWW_ROOT);
            // 2.添加请求-处理函数映射关系
            _srv.Post("/video", Insert);
            _srv.Delete("/video/(\\d+)", Delete); // 正则表达式 ,  将delete绑定到 /video/<id>上
            _srv.Put("/video/(\\d+)", Update);
            _srv.Get("/video/(\\d+)", SelectOne);

            _srv.Get("/video", Select);
            // 3.启动服务器
            cout << "======启动成功======" << endl;
            _srv.listen("0.0.0.0", _port);

            return true;
        }
    };

}
