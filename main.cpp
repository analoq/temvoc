#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <sndfile.hh>
#include "TemVoc.hpp"

const string VERSION = "1.0";

using namespace std;

int main(int argc, char *argv[]) try
{
  string help_message {"temvoc [-hwv] [-f fftsize] <carrier path> <modulator path> <output path>"};
  bool weighting = false;
  int fftsize {1024};
  char c;
  while ((c = getopt (argc, argv, "vhwf:")) != -1)
  {
     double base;
     switch (c)
     {
       case 'v':
         cerr << "temporal vocoder (c)2014 analoq[labs]" << endl;
         cerr << "version: " << VERSION << endl;
         return 0;

       case 'h':
         cerr << help_message << endl
              << "  -v               Display version information\n"
                 "  -h               Displays help\n"
                 "  -w               Apply A-Weighting during matching\n"
                 "  -f <size>        FFT size (powers of 2)\n"
                 "  <carrier path>   Path to Carrier audio file\n"
                 "  <modulator path> Path to Modulator audio file\n"
                 "  <output path>    Path to output audio file\n";
         return 0;

       case 'w':
         weighting = true;
         break;

       case 'f':
         try
         {
           fftsize = stoi(optarg);
         }
         catch (invalid_argument &e)
         {
           cerr << "FFT Size could not be parsed" << endl;
           return 1;
         }
         if ( fftsize < 64 || fftsize > 8192 )
         {
           cerr << "FFT Size must be between 64 and 8192." << endl;
           return 1;
         }
         base = log(fftsize)/log(2);
         if ( floor(base) != base )
         {
           cerr << "FFT Size must be a power of 2." << endl;
           return 1;
         }
         break;

       case '?':
         if (optopt == 'f')
           cerr << "Option -" << (char)optopt << " requires an argument." << endl;
         else if (isprint (optopt))
           cerr << "Unknown option `-" << (char)optopt << "'." << endl;
         else
           cerr << "Unknown option character `\\x" << optopt << "'." << endl;
         return 1;

       default:
         cerr << help_message << endl;
     }
  }

  if (argc - optind < 3)
  {
    cerr << help_message << endl;
    return 1;
  }
  string carrier_path {argv[optind]};
  string modulator_path {argv[optind+1]};
  string output_path {argv[optind+2]};
    
  // load input files
  SndfileHandle carrier{carrier_path};
  if ( carrier.error() != SF_ERR_NO_ERROR )
  {
    cerr << "Could not load '" << carrier_path << "' - " << carrier.strError() << endl;
    return 1;
  }

  SndfileHandle modulator{modulator_path};
  if ( modulator.error() != SF_ERR_NO_ERROR )
  {
    cerr << "Could not load '" << modulator_path << "' - " << modulator.strError() << endl;
    return 1;
  }

  if ( carrier.channels() != 1 || modulator.channels() != 1 )
  {
    cerr << "Input audio files must be be mono" << endl;
    return 1;
  }
  
  if (modulator.samplerate() != carrier.samplerate())
  {
    cerr << "Carrier and Modulator sample rates must match." << endl;
    return 1;
  }

  cerr << "Carrier:     " << carrier_path << endl
       << "Modulator:   " << modulator_path << endl
       << "Output:      " << output_path << endl
       << "FFT Size:    " << fftsize << endl
       << "A-Weighting: " << (weighting ? "On" : "Off") << endl;

  TemVoc temvoc {fftsize, weighting, carrier, modulator};

  temvoc.process(output_path, [](double p)
  {
    cerr << "Processing "
         << fixed << setprecision(0)
         << 100*p << "% \r";
  });

  cerr << endl;
  return 0;
}
catch (exception &e)
{
  cerr << "Error: " << e.what() << endl;
  return -1;
}
