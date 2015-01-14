#include <iostream>
#include <vector>
#include <utility>
#include <exception>

using namespace std;

struct CLIException : runtime_error
{
public:
  CLIException(string e) : runtime_error{e} {}
};

class CLIOption
{
private:
  string description;

protected:
  string name;
  string value;

public:
  CLIOption(string n, string d)
    : name {n}, description{d}, value{""}
  {
  }
  
  void setValue(string v)
  {
    value = v;
  }
  
  string getValue()
  {
    return value;
  }
  
  string getDescription()
  {
    return description;
  }
  
  virtual string getName()
  {
    return name;
  }
  
  virtual void validate()
  {
  }
};

class CLIRequiredOption : public CLIOption
{
public:
  CLIRequiredOption(string n, string d) : CLIOption{n, d}
  {
  }

  string getName()
  {
    return "<" + name + ">";
  }
};

class CLINamedOption : public CLIOption
{
private:
  string long_name;
  bool has_argument;
public:
  CLINamedOption(string n, string ln, bool ha, string d)
    : long_name{ln}, has_argument{ha}, CLIOption{n, d}
  {
  }
  
  bool hasArgument()
  {
    return has_argument;
  }

  bool isSet()
  {
    return value != "";
  }
  
  string getName()
  {
    return "-" + name;
  }
  
  string getLongName()
  {
    return "--" + long_name;
  }
};

class CLI
{
private:
  vector<CLIOption *> options;
  string description;

public:
  CLI(string d) : description{d} {}
  
  void addOption(CLIOption& option)
  {
    options.push_back(&option);
  }

  void process(int argc, char *argv[])
  {
    // fill in option values
    for (int i = 0; i < argc-1; i ++)
    {
      string arg {argv[i+1]};
      // check for named options
      for (auto option : options)
      {
        CLINamedOption *named_option {dynamic_cast<CLINamedOption *>(option)};
        if ( named_option )
        {
          if (arg == named_option->getName() || arg == named_option->getLongName())
          {
            if (named_option->hasArgument())
            {
              i ++;
              if ( i >= argc )
                throw CLIException{"missing argument for " + arg};
              named_option->setValue(argv[i+1]);
              named_option->validate();
            }
            else
              named_option->setValue("SET");
            break;
          }
        }
        else if (option->getValue() == "")
        {
          option->setValue(arg);
          option->validate();
          break;
        }
      }
    }
  }
  
  void printMessage()
  {
    cerr << description << endl;
  }
  
  void printHelpMessage()
  {
    printMessage();
    for (auto option : options)
    {
      cerr << "   " << option->getName();

      CLINamedOption *named_option {dynamic_cast<CLINamedOption *>(option)};
      if ( named_option )
        cerr << " " << named_option->getLongName();

      cerr << endl << "      " << option->getDescription() << endl;
    }
  }
};
