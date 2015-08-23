// resizing vector
#include <iostream>
#include <vector>
#include <climits>
using namespace std;
int main ()
{
  std::vector<int> myvector;

  // set some initial content:
  unsigned long long int one=1;
  unsigned long long int p=(one<<63);
  cout<<p<<endl;
    cout<<ULLONG_MAX<<endl;
        
  return 0;
}