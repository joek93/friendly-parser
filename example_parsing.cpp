#include <iostream>
#include <vector>
#include <string>
#include "parser.hpp"

using namespace std;


class arg_list : public ArgListBase{
public:
    arg_list() {
        num_fixed_args = 1;
        //TODO: make assignments somewhat more intuitive
        variables = { // first type must always be a string!
            std::make_pair<string,string>("fix_string",""),
            std::make_pair<string,int>("opt_int1",0),
            std::make_pair<string,int>("opt_int2",1),
            std::make_pair<string,double>("opt_doub",10.0),
            std::make_pair<string,bool>("opt_bool",false),
        };
    }
    int sanity_checks(){
        // managed by the user 
        // by convention should return 0 if everything ok, 
        // or the variable index + 1 if not ok.
        
        int ret = 0;

        if((ret = check_var<string>([](string str){ 
                if(str.empty()){ cout<<"ERROR: empty fix_string"<<endl;}
                return str.empty();},"fix_string"))) 
            return ret;

        ret = check_var<int>([](int val){ 
                if(val<0){ cout<<"ERROR: negative opt_int1"<<endl;}
                return val<0;},"opt_int1");

        return ret;
    }

};


int main(int argc, char** argv){

    // calling without enough fixed arguments prints the usage line

    arg_list args;
    if(parse_arguments(args, argc, argv)!=0){
      exit(1);
    }

    cout<<"arguments:\n"<<args<<endl;

    // you can access the arguments both to read their values 
    // and for printing them (not writable in the api) as follows:
    // string fix_string = args["fix_string"];
    // cout<<args["opt_doub"]<<endl;;

    return 0;
}
