#include <iostream>
#include <ctime>
#include <chrono>
#include "TinyWebServer/log.h"
#include "TinyWebServer/configurator.h"
#include <thread>
#include <mutex>
#include <filesystem>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
using namespace std;
using namespace WebSrv;
// 测试配置器功能

// int
static ConfigVarBase::ptr g_intValue = Configurator::lookup("variable.int", (int)8080, "variable int");

// float
static ConfigVarBase::ptr g_floatValue = Configurator::lookup("variable.float", (float)3.1415f, "variable float");

// double
static ConfigVarBase::ptr g_doubleValue = Configurator::lookup("variable.double", (double)3.14, "variable double");

// Repeated unexpected
static ConfigVarBase::ptr g_reIntValue = Configurator::lookup("variable.int", (float)3.14, "variable int");

static ConfigVarBase::ptr g_intVec = Configurator::lookup("variable.int_vec", vector<int>{1, 2, 3}, "variable int_vec");

static ConfigVarBase::ptr g_intSet = Configurator::lookup("variable.int_set", set<int>{2, 3, 4}, "variable int_set");

static ConfigVarBase::ptr g_stirngIntMap = Configurator::lookup("variable.string_int_map", map<string, int>{{"A", 1}, {"B", 2}}, "variable string_int_map");

static ConfigVarBase::ptr g_intUnorderedSet = Configurator::lookup("variable.int_unordered_set", unordered_set<int>{4, 5, 6}, "variable int_unordered_set");

static ConfigVarBase::ptr g_stringIntHashNap = Configurator::lookup("variable.string_int_unordered_map", unordered_map<string, int>{{"A", 1}, {"B", 2}}, "variable string_int_unordered_map");

// 打印配置值
void printConfig(ConfigVarBase::ptr ptr)
{
    cout << ptr->getName() << ":" << endl;
    cout << ptr->toString() << endl;
}

// 加载配置
void loadYamlConfig()
{
    cout << "load yaml file config before:" << endl;
    printConfig(g_intVec);
    printConfig(g_intSet);
    printConfig(g_stirngIntMap);
    printConfig(g_intUnorderedSet);
    printConfig(g_stringIntHashNap);
    Configurator::loadConfigFile("/home/ubuntu/work/TinyWebServer/tests/configurator_test/test.yaml");
    cout << "load yaml file config after:" << endl;
    printConfig(g_intVec);
    printConfig(g_intSet);
    printConfig(g_stirngIntMap);
    printConfig(g_intUnorderedSet);
    printConfig(g_stringIntHashNap);
}

// 对类特化

class Person
{
public:
    Person()=default;
    Person(std::string n,int a,char s):name(n),age(a),sex(s){

    }

    void print() const
    {
        cout << "Name: " << name << '\n'
             << "age: " << age << '\n'
             << "sex: " << sex << endl;
    }
    
    bool operator==(const Person &p) const
    {
        return p.age == age && p.name == name && p.sex == sex;
    }
    bool operator<(const Person &p) const
    {
        return name < p.name;
    }
    bool empty(){
        return name.empty();
    }

    std::string name;
    int age;
    char sex;
};


template <>
class WebSrv::LexicalCast<Person, std::string>
{
public:
    Person operator()(const std::string &v)
    {
        YAML::Node node = YAML::Load(v);
        Person p;
        p.name=node["name"].as<std::string>();
        p.age=node["age"].as<int>();
        p.sex=node["sex"].as<char>();
        return p;
    }
};


template <>
class WebSrv::LexicalCast<std::string,Person>
{
public:
    std::string operator()(const Person &v)
    {
        YAML::Node node;
        Person p;
        node["name"]=v.name;
        node["age"]=v.age;
        node["sex"]=v.sex;
        std::stringstream ss;;
        ss<<node;
        return ss.str();
    }
};

ConfigVar<Person>::ptr g_person=Configurator::lookup("person.person",Person());
ConfigVar<std::list<Person>>::ptr g_personList=Configurator::lookup("person.list",std::list<Person>());
ConfigVar<std::map<std::string,Person>>::ptr g_personMap=Configurator::lookup("person.map",std::map<std::string,Person>());
ConfigVar<std::set<Person>>::ptr g_personSet=Configurator::lookup("person.set",std::set<Person>());

std::list<Person> persons;
// 处理配置
void process(){
    cout<<"--------------Person process---------------"<<endl;

    cout<<"##add callback in change print"<<endl;
    g_person->addChangeValueListener([](const Person &oldValue, const Person &newValue){
        cout<<"old Value:"<<endl;
        oldValue.print();
        cout<<"new Value:"<<endl;
        newValue.print();
    });
    g_personList->addChangeValueListener([](const std::list<Person> &oldValue, const std::list<Person> &newValue){
        cout<<"old Value:"<<endl;
        for (auto &&i : oldValue)
        {
            i.print();
        }
        cout<<"new Value:"<<endl;
        for (auto &&i : newValue)
        {
            i.print();
        }
    });

    g_personMap->addChangeValueListener([](const std::map<std::string,Person> &oldValue, const std::map<std::string,Person> &newValue){
        cout<<"old Value:"<<endl;
        for (auto &&i : oldValue)
        {
            cout<<i.first<<":"<<endl;
            i.second.print();
        }
        cout<<"new Value:"<<endl;
        for (auto &&i : newValue)
        {
            cout<<i.first<<":"<<endl;
            i.second.print();
        }
    }
    );

    g_personSet->addChangeValueListener([](const std::set<Person> &oldValue, const std::set<Person> &newValue){
        cout<<"old Value:"<<endl;
        for (auto &&i : oldValue)
        {
            i.print();
        }
        cout<<"new Value:"<<endl;
        for (auto &&i : newValue)
        {
            i.print();
        }
    });

    Configurator::loadConfigFile("/home/ubuntu/work/TinyWebServer/tests/configurator_test/test2.yaml");

    cout<<"##using g_personList setValue change global list<Person>"<<endl;


   g_personList->addChangeValueListener([](const std::list<Person> &oldValue, const std::list<Person> &newValue){
        persons.insert(persons.end(),newValue.begin(),newValue.end());
   });

    g_personList->delChangeValueListener(1);
    
    persons=g_personList->getValue();
    g_personList->setValue(std::list<Person>{Person("zhaoliu",21,'f'),Person("laowang",32,'m'),});
    for(auto &&i: persons){
       i.print();
    }
    
}

int main()
{
    
    loadYamlConfig();
    process();
    cout<<"Configure all variables and print them"<<endl;
    Configurator::visit(printConfig);
    YAML::Node node;
    node["name"]="zhangshan";
    node["sex"]="";
    cout<<node<<endl;

}