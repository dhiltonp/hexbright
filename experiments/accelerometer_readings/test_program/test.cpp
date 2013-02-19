#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdlib>

#include "../../../libraries/hexbright/hexbright.h"

using namespace std;


std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    return split(s, delim, elems);
}

class hbtest : public hexbright {
private:
  std::vector<std::vector<int> > accelerometer_data;
  int accelerometer_location;
public:
  hbtest(string file) {
    accelerometer_data = std::vector<std::vector<int> >();
    accelerometer_location = 0;

    // read accelerometer_data from file
    ifstream f;
    string line;
    f.open(file.c_str());
    while(!f.eof()) {
      getline(f, line);
      if(line.find("recorded vector")==string::npos) {
        continue;
      } else {
        std::vector<string> tmp;
        tmp = split(line, ' ');
        tmp = split(tmp[1], '/');
        std::vector<int> vec;
        
        for(int i=0; i<3; i++) {
          vec.push_back(atoi(tmp[i].c_str()));
        }
        accelerometer_data.push_back(vec);
      }
    }

    // reset accelerometer_location
    accelerometer_location = 0;

    // pre-load accelerometer buffers
    int * data = &(accelerometer_data[accelerometer_location])[0];
    hexbright::fake_read_accelerometer(data);
    hexbright::fake_read_accelerometer(data);
    hexbright::fake_read_accelerometer(data);
    hexbright::fake_read_accelerometer(data);
  }
  void read_accelerometer() {
    int * data = &(accelerometer_data[accelerometer_location])[0];
    hexbright::fake_read_accelerometer(data);
    accelerometer_location++;
  }
  bool data_exists() {
    if(accelerometer_location<accelerometer_data.size())
      return true;
    return false;
  }
};


void test_accelerometer(string file) {
  hbtest hb(file);
  int spinned = 0;
  while (hb.data_exists()) {
    hb.find_down();
    hb.read_accelerometer();
    char vector_buffer[30];
    sprintf(vector_buffer, "\t    %4d %4d %4d", hb.vector(0)[0], hb.vector(0)[1], hb.vector(0)[2]);
    string vec0 = vector_buffer;
    sprintf(vector_buffer, "\t    %4d %4d %4d", hb.down()[0], hb.down()[1], hb.down()[2]);
    string down = vector_buffer;
    cout<<(int)hb.get_spin()<<"\t"<<(int)hb.magnitude(hb.vector(0))<<vec0<<down<<endl;
    //cout<<(int)hb.magnitude(hb.vector(0))<<endl;
    spinned += hb.get_spin();
    
    // hb.print_vector(hb.vector(0), "vector read");
    //hb.print_vector(hb.down(), "estimated down"); // normalized, btw
  }
  cout<<"  total rotation: "<<spinned<<endl;
  cout<<"------------------------------"<<endl;
}

int main(int argc, char** argv) {
  for(int i=1; i<argc; i++) {
    cout<<"file: "<<argv[i]<<endl;
    test_accelerometer(argv[i]);
  }
  if(argc==1) {
    cout<<" usage: "<<argv[0]<<" file1 file2 ..."<<endl;
  }
  return 0;
}
