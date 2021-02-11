#include <iostream>
#include <algorithm>
#include <vector>
#include <variant>
#include <utility>
#include <string>
#include <functional>
#include <map>

using namespace std;


struct ToggleFlag{
    bool val=false;
    ToggleFlag(bool val) : val(val){}
    friend ostream& operator<<(ostream& o, const ToggleFlag& tf);
};

ostream& operator<<(ostream& o, const ToggleFlag& tf){
    o<<((tf.val)? "(set)" : "(unset)");
    return o;
}

typedef std::variant<string,int,float,double,bool,ToggleFlag> MultiType;

std::ostream& operator<<(std::ostream& os, const MultiType& v){
    std::visit([&os](auto&& arg) {
        string arg_string;
        using T = std::decay_t<decltype(arg)>;
        if constexpr(std::is_same_v<T,bool>){
            arg_string=(arg)?"true":"false";
        }else if constexpr(std::is_same_v<T,string>){
            arg_string=arg;
        }else if constexpr(std::is_same_v<T,int> //TODO: check just numeric
                || std::is_same_v<T,float> 
                || std::is_same_v<T,double>){
            arg_string=to_string(arg);
        }
        os << arg_string;
    }, v);
    return os;
}

class ArgListBase{
protected:
    int num_fixed_args = 1;
    std::vector<std::pair<string,MultiType>> variables;
    std::map<std::string,int> index_map; 
    //TODO: add parameter descriptions to be printed in usage
public:

    
    ArgListBase(){} 

    void check_fill_index_map(){
        if(index_map.empty()){
            int idx=0;
            for(const auto& el: variables){
                index_map[el.first] = idx++;
            }  
        }
    }

    friend ostream& operator<<(ostream& o, ArgListBase& al);

    inline int get_num_fixed_args(){ return num_fixed_args; }
    inline int get_num_floating_args(){ return variables.size()-num_fixed_args; }
    inline string get_var_name(int idx){ return variables[idx].first; }

    class proxy {
        friend class ArgListBase;
    protected:
        MultiType& it;
        proxy(MultiType& it): it(it) {}
    public:
        template<typename T>
        operator T() {
            T val = std::get<T>(it);
            return val;
        }

        friend std::ostream& operator<<(std::ostream& os, const proxy& pr);
    };

    MultiType& get_var(int idx){
        check_fill_index_map();
        return variables[idx].second;
    }
    MultiType& get_var(string name){
        check_fill_index_map();
        return variables[index_map[name]].second;
    }

    proxy operator[](int idx){
        return proxy(variables[idx].second);
    }
    proxy operator[](string name){
        check_fill_index_map();
        return proxy(get_var(name));
    }
    proxy operator[](const char* cname){
        string name = cname;
        return operator[](name);
    }

    string usage_string(){
        string ret= " ";
        for(int i=0; i<num_fixed_args; ++i) 
            ret += "<"+variables[i].first+"> ";
        for(size_t i=num_fixed_args; i<variables.size(); ++i){
            ret += "[--"+variables[i].first;
//          if constexpr(std::is_same_v<decltype(variables[i].second),ToggleFlag>)
//              ret+="]\n";
//          else
            string arg_string;
            std::visit( [&arg_string](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr(std::is_same_v<T,bool>){
                    arg_string=(arg)?"true":"false";
                }else if constexpr(std::is_same_v<T,string>){
                    arg_string=arg;
                }else if constexpr(std::is_same_v<T,int> 
                        || std::is_same_v<T,float>
                        || std::is_same_v<T,double>){
                    arg_string=to_string(arg);
                }
            }, variables[i].second);
            ret+=((arg_string.empty())? "":" ("+arg_string+")")+"] "; 

        }
        return ret;
    }

    template <typename T>
    int check_var(const std::function<bool(T)>& lambda, string name){
        int idx = index_map[name];
        T var = operator[](idx);
        bool incorrect = lambda(var);
        return (incorrect) ? 1+idx : 0;
    }
    template <typename T>
    int check_var(const std::function<bool(T)>& lambda, const char* cname){
        string name = cname;
        return check_var<T>(lambda, name);
    }

    virtual int sanity_checks() {return 0;}; 

    void clear(){index_map.clear();}
};

ostream& operator<<(ostream& o, ArgListBase& al){
    for(size_t i=0; i<al.variables.size(); ++i){
        o<<al.get_var_name(i)<<": "<<al.get_var(i)<<endl;
    }
    return o;
}

std::ostream& operator<<(std::ostream& os, const ArgListBase::proxy& pr){
    os<<pr.it;
    return os;
}

void str2argtype(MultiType& varvar, const string& str,const string& var_name){
    bool error=false;
    std::visit( [str,&error](auto&& var) {
    using T = std::decay_t<decltype(var)>;
    if constexpr(std::is_same_v<T,string>){
        var = str;
    }else if constexpr(std::is_same_v<T,float>){
        var = stof(str.c_str(),NULL); 
    }else if constexpr(std::is_same_v<T,double>){
        var = stod(str.c_str(),NULL); 
    }else if constexpr(std::is_same_v<T,int>){
        var = atoi(str.c_str()); 
    }else if constexpr(std::is_same_v<T,bool>){
        if(str.compare("true")==0 or str.compare("1")==0){
            var = true;
        }else if(str.compare("false")==0 or str.compare("0")==0){
            var = false;
        }else{
            error=true; 
//            throw std::logic_error(("ERROR:
        }
    }else{
        error=true;
    }
    },varvar);
    if(error){
       throw std::logic_error(("ERROR: unsupported type for "+var_name+" in str2argtype conversion").c_str()); 
    }
}


void parse_floating_terms(ArgListBase& args, int argc, map<string,int>& argmap, map<int,string>& argmap_inv){
    const int NFixA = args.get_num_fixed_args();
    const int NFloA = args.get_num_floating_args();
    for(int ai = 0; ai< NFloA; ++ai){
        string var_name = "--"+args.get_var_name(ai+NFixA);
        int tmp_idx = argmap[var_name]; // find index in command line parsing
        if(tmp_idx>=args.get_num_fixed_args()){
            if(tmp_idx+1>= argc)
                throw std::runtime_error(("ERROR: set value after '"+var_name+"' flag").c_str()); 
            
            str2argtype(args.get_var(ai+NFixA),argmap_inv[tmp_idx+1],var_name); 

        }
    }
}


template <class ArgList>
int parse_arguments(ArgList& args, int argc, char** argv){
    map<string,int> argmap;
    map<int,string> argmap_inv;
////    char *end;
////    int base_strtoull = 10;

    args.clear();


    if(argc<=args.get_num_fixed_args()){
      cout<<"usage:\n"<<argv[0]<<" "<<args.usage_string()<<endl;
      return -1;
    }

    // fixed arguments
    for(int i = 0; i < args.get_num_fixed_args(); ++i){
        str2argtype(args.get_var(i),argv[1+i],args.get_var_name(i));
    }

    // collect floating arguments
    for(int i = args.get_num_fixed_args(); i < argc; ++i){
        argmap[argv[i]]=i;
        argmap_inv[i]=argv[i];
    }

    parse_floating_terms(args, argc, argmap, argmap_inv);


    int ret = args.sanity_checks();
    if(ret){
        throw ("ERROR: argument <"+args.get_var_name(ret-1)+"> invalid").c_str();
    }

    return ret;
}
