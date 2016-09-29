#include <cstdlib>
#include <iostream>
#include <cstring>
#include <sys/stat.h>
#include <sstream>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <limits.h>
#include <string>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
using namespace std;


vector<char *> mk_cstrvec(vector<string> & strvec);
void dl_cstrvec(vector<char *> & cstrvec);
void nice_exec(vector<string> args);
inline void nope_out(const string & sc_name);
void help(void);
void pipeCommands(vector<vector<string>> vectors, int numPipes, bool isSign, vector<string> sign, vector<string> files);
void makevvs(vector<string> commands, int numPipes, bool isSign, vector<string> sign, vector<string> files);
void defaultSignal(void);
void jobs(vector<string> jv, int jcount);
string createJobs(vector<string> sv, int numPipes);

void redirect(vector<string> sign, vector<string> files);

struct jobs{
  string status;
  string command;
  int jid;
  pid_t pid;
};

int main(){

  signal(SIGINT, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);
  int jcount = 0;
  bool bgCommand = false;
  /*  int numJobs = 1000;
  jobs job;
  jobs jobList[1000];*/
  while(true){


    stringstream ss;
    string userInput;
    string n;
    string pipe = "|";
    string input, output, error, quotestr;
    int numPipes = 0;

    char * path;
    char * buffer;
    vector<string> sign;
    vector<string> stringVec;
    vector<string>::iterator vecIter;
    vector<string> files;


    bool  isSign = false,
      stdin = false,
      stdout = false,
      stdoutApp = false,
      stdErr = false,
      stdErrApp = false,
      quotestring = false;

    input = "STDIN_FILENO";
    output = "STDOUT_FILENO";
    error = "STDERR_FILENO";


    path = getenv("HOME");
    cout << "1730sh:";
    if((buffer = get_current_dir_name()) != nullptr) {
      if(strlen(path) <= strlen(buffer)) {
	int x = (strlen(buffer) - strlen(path));
	cout << '~';
	for(int i = strlen(buffer) - x; i < (int) strlen(buffer); i++) {
	  cout << buffer[i];
	}
	cout << "/$ ";
      } else {
	cout << buffer <<"/$ ";
      }
      free(buffer);
    }

    getline(cin,userInput);
    //if(userInput.compare("quit") == 0)break;
    if(userInput.empty()){
      continue;
    }
    ss << userInput;

    /** PARSING USER INPUT. This parses the input and puts it into a string vector.
     *	From there
     *  it looks for quotation marks and pipes to properly separate each process
     *  argument(s).
     */
    while(ss >> n) {

      if(n.compare("|") == 0){
	numPipes++;
      } //number of pipes counter

      if(n.compare("&") == 0){
	bgCommand = true;
	continue;
      }

      if(n.compare("<") == 0) {
	stdin = true;
	isSign = true;
	sign.push_back("<");
	continue;
      } //input

      if(n.compare(">") == 0) {
	stdout = true;
	isSign = true;
	sign.push_back(">");
	continue;
      } //output


      if(n.compare(">>") == 0) {
	stdoutApp = true;
	isSign = true;
	sign.push_back(">>");
	continue;
      } // output append

      if(n.compare("e>") == 0) {
	stdErr = true;
	isSign = true;
	sign.push_back("e>");
	continue;
      } // error

      if(n.compare("e>>") == 0) {
	stdErrApp = true;
	isSign = true;
	sign.push_back("e>>");
	continue;
      } //error append

      if(stdin) {
	input = n;
	files.push_back(n);
	stdin = false;
	continue;
      } // if

      if(stdout) {
	output = n + " (truncate)";
	files.push_back(n);
	stdout = false;
	continue;
      } // if

      if(stdoutApp) {
	output = n + " (append)";
	files.push_back(n);
	stdoutApp = false;
	continue;
      } // if

      if(stdErr) {
	error = n + " (truncate)";
	files.push_back(n);
	stdErr = false;
	continue;
      } // if

      if(stdErrApp) {
	error = n + " (append)";
	files.push_back(n);
	stdErrApp = false;
	continue;
      } // if

      if(n.at(0) == '"') {
	quotestr = n;
	quotestr.push_back(' ');
	quotestring = true;
	continue;
      } // if

      if(n.at(n.length() - 1) == '\\') {
	n.pop_back();
	n.pop_back();
	n.push_back('"');
      } // if

      if(quotestring) {
	quotestr += n;
	quotestr.push_back(' ');
      } // if

      if (n.at(n.length() - 1) == '"' &&  n.at(n.length() - 2) != '\\' ) {
	quotestr.erase(remove(quotestr.begin(), quotestr.end(), '\\'), quotestr.end());
	quotestr.pop_back();
	quotestr.pop_back();
	quotestr = quotestr.substr(1, quotestr.length());
	stringVec.push_back(quotestr);
	quotestring = false;
	continue;
      } // if
      if(!quotestring) stringVec.push_back(n);

    } // while

    // BUILT INS;
    vecIter = stringVec.begin();


    if(*vecIter == "cd"){
      vecIter++;
      string filename = *vecIter;
      if(filename[0] == '~'){
	string relative = path;
	relative+=filename.substr(1,filename.size());
	if(chdir(relative.c_str()) == -1) perror("cd");
      } // if

      else{
	if(chdir(filename.c_str()) == -1) perror("cd");
      } // else
    } // if
    else if(*vecIter == "jobs"){
      if(bgCommand &&( numPipes == 0)){
	jobs(stringVec, jcount);
      }
      else if(bgCommand && (numPipes > 0)){
	createJobs(stringVec, numPipes);
      }
      else{
	cout<<"No Jobs to print"<<endl;
      }

    }
    else if(*vecIter == "exit"){
      if(stringVec.size() > 1){
	vecIter++;
	string status = *vecIter;
	exit(stoi(status));
      } // if
      else if ( stringVec.size() == 1){
	exit(getpid());
      } // else if
    }  // else if
    else if(*vecIter == "help"){
      if(stringVec.size() == 1){
	help();
      } // if
    } // else if
    else{
      pid_t pid;
      if(numPipes == 0){
	if((pid = fork()) == -1){
	  nope_out("fork");
	} // if for numPipes
	else if(pid == 0){ /* child stuff */
	  defaultSignal();
	  // stringstream delimiting |. exec each process.
	  if(numPipes == 0){
	      if(isSign){
		redirect(sign,files);
	      }
	      nice_exec(stringVec);
	  }
	} // else if
 	else{ /* parent stuff */
	   waitpid(pid,nullptr,0);
	} // else
      } // else for forking.
      else if(numPipes > 0){
	//MAKE THE VECTOR OF VECTORS
	makevvs(stringVec,numPipes,isSign,sign,files);
      } // else if for numPipes>0
    }
  }// while


  return EXIT_SUCCESS;
} // main



vector<char *> mk_cstrvec(vector<string> & strvec) {

  vector<char *> cstrvec;
  
  for (unsigned int i = 0; i < strvec.size(); ++i) {
    cstrvec.push_back(new char [strvec.at(i).size() + 1]);
    strcpy(cstrvec.at(i), strvec.at(i).c_str());
  } // for
  cstrvec.push_back(nullptr);
  return cstrvec;
} // mk_cstrvec

void dl_cstrvec(vector<char *> & cstrvec) {
  for (unsigned int i = 0; i < cstrvec.size(); ++i) {
    delete[] cstrvec.at(i);
  } // for
} // dl_cstrvec

void nice_exec(vector<string> strargs) {
  vector<char *> cstrargs = mk_cstrvec(strargs);
  execvp(cstrargs.at(0), &cstrargs.at(0));
  perror("execvp");
  dl_cstrvec(cstrargs);
  exit(EXIT_FAILURE);
} // nice_exec


/** ERROR MESSAGES. */
inline void nope_out(const string & sc_name) {
  perror(sc_name.c_str());
  exit(EXIT_FAILURE);
} // nope_out

/* default signals for childs */
void defaultSignal(void){
  signal(SIGINT, SIG_DFL);
  signal(SIGQUIT, SIG_DFL);
  signal(SIGTSTP, SIG_DFL);
  signal(SIGTTIN, SIG_DFL);
  signal(SIGTTOU, SIG_DFL);
  //   signal(SIGCHLD, SIG_DFL);
}// default Signal

void help(void){
  cout << "Available Commands: \n"
       << "bg JID             : Resume the stopped job JID in the background, as if it had been started with &.\n"
       << "cd [PATH]          : Change the current directory to PATH. The environmental variable HOME is the default PATH.\n"
       << "exit [N]           : Cause the shell to exit with a status of N. If N is omitted, the exit status is that of the last job executed.\n"
       << "export NAME[=WORD] : NAME is automatically included in the environment of subsequently executed jobs.\n"
       << "fg JID             : Resume job JID in the foreground, and make it the current job.\n"
       << "help               : Display helpful information about builtin commands.\n"
       << "jobs               : List current jobs."<< endl;

} // help built-in function.

void makevvs(vector<string> commands, int numPipes, bool isSign, vector<string> sign, vector<string> files){
  string thePipe = "|";
  vector<vector<string>> mainsvv;
  unsigned int counter = 0;


  for(int i =0;i<(numPipes+1);i++){
    vector<string> temp = {};

    while(counter<commands.size()){
      if(thePipe.compare(commands[counter]) == 0){
	counter++;
	break;
      }
      else{
	temp.push_back(commands[counter]);
	counter++;
      }
    }
    mainsvv.push_back(temp);
  }
  pipeCommands(mainsvv,numPipes, isSign, sign, files); // if temp3 = less then idk how to run it properly.
}

void pipeCommands( vector<vector<string>> vectors, int numPipes, bool isSign, vector<string> sign, vector<string> files){

  int length = 2*numPipes;
  int * pipefd = new int[length];
  pid_t pid;

  for(int i =0; i < numPipes; i++){
    if(pipe(pipefd + i*2) == -1) nope_out("pipe");
  }

  for(unsigned int i =0;i<vectors.size();i++){
    pid = fork();
    if(pid == -1) nope_out("fork");
    else if(pid == 0){
      defaultSignal();
      if(i!=0){ // if its not first command, dup the input
	if(dup2(pipefd[(i-1)*2],0) == -1) nope_out("dup2");
      }
      if(i!=(vectors.size()-1)){ // if its not last command, dup the output
	if(dup2(pipefd[(i*2)+1],1) == -1) nope_out("dup2");
      }
      //close pipes
      for(int j = 0;j<numPipes*2;j++){
	close(pipefd[j]);
      }
      if(isSign && (i==vectors.size()-1)){ // if at last argument
	redirect(sign,files);
      }
      nice_exec(vectors[i]); // execute each command
    }
  } // for

  /* parents time */
  for(int i =0;i<2*numPipes;i++){
    close(pipefd[i]);
  }

  for(int i =0;i<numPipes+1;i++){
    waitpid(pid,nullptr,0);
  }

  delete [] pipefd;
}

void redirect(vector<string> sign, vector<string> files){

  if(sign.size() != files.size()) nope_out("redirection");

  for(unsigned int i =0;i<sign.size();i++){
    int fd;
    if(sign[i] == ">"){ // truncate
      if((fd = creat(files[i].c_str(),0644))<0) nope_out("creat");
      dup2(fd,1);
      close(fd);

    }
    if(sign[i] == ">>"){ // append
      if((fd = open(files[i].c_str(),O_WRONLY | O_APPEND | O_CREAT,0644))<0) nope_out("open");
      dup2(fd,1);
      close(fd);

    }
    if(sign[i] == "<"){ // input
      if((fd = open(files[i].c_str(),O_RDONLY,0))<0) nope_out("open");
      dup2(fd,0);
      close(fd);

    }
    if(sign[i] == "e>"){ // error truncate
      if((fd = creat(files[i].c_str(),0644))<0) nope_out("creat");
      dup2(fd,2);
      close(fd);

    }
    if(sign[i] == "e>>"){ // error append
      if((fd = open(files[i].c_str(),O_WRONLY | O_APPEND | O_CREAT,0644))<0) nope_out("open");
      dup2(fd,2);
      close(fd);

    }
  }
}

void jobs(vector<string> jv, int jcount) {
  vector<string>::iterator jvit;
  if(!jv.empty()) {
    jvit = jv.begin();
    cout << "JID     STATUS     COMMAND" << endl;
    for(; jvit != jv.end(); jvit++) {
      string j = *jvit;
      cout << jcount
	   << "       "
	   << "Running"
	   << "       "
	   << j
	   << endl;
      jcount++;
    }
  }
  else
    cout << "No jobs currently running." << endl;
}// jobs

string createJobs(vector<string> sv, int numPipes) {
  string pipeS = "|";
  int numArgs = numPipes + 1;
  int pipeCount = 0;
  string j;
  int count;
  vector<string>::iterator it;
  it = sv.begin();
  for(int i = 0; i < numArgs; i++) {
    count = 0;
    for(; it != sv.end(); it++) {
      if(pipeS.compare(*it) != 0 && pipeCount <= numPipes) {
	j += *it;
	j += " ";
      } else {
	it++;
	pipeCount++;
	break;
      }// if else
      count++;
    }//for
  }// for
  return j;
}// createJobs
