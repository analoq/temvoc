#include <vector>
#include <limits>
#include <functional>
#include <sndfile.hh>
#include <kiss_fftr.h>

using namespace std;

class TemVoc
{
private:
  const int fft_size;  
  const int sample_rate;
  bool apply_weighting;
  kiss_fft_scalar *weighting_envelope;

  SndfileHandle carrier;
  SndfileHandle modulator;

  static double norm(const kiss_fft_cpx cpx)
  {
    return sqrt(cpx.r*cpx.r + cpx.i*cpx.i);
  }

  static double weighting(const double f)
  {
    return pow(12200, 2) * pow(f, 4) /
           ( (pow(f, 2) + pow(20.6, 2))
             * (pow(f, 2) + pow(12200, 2)) 
             * sqrt( (pow(f, 2) + pow(107.7, 2)) * (pow(f, 2) + pow(737.9, 2)))
           );
  }

  double diff(const kiss_fft_cpx *v1, const kiss_fft_cpx *v2)
  {
    double delta{0.0};
    for (int i = 0; i < fft_size/2+1; i ++)
      delta += weighting_envelope[i]*fabs(norm(v1[i]) - norm(v2[i]));
    return delta;
  }

  kiss_fft_cpx *find(const kiss_fft_cpx *carrier, const vector<kiss_fft_cpx *> modulator)
  {
    double lowest{numeric_limits<double>::max()};
    double delta;
    kiss_fft_cpx *closest = NULL;

    for (kiss_fft_cpx *fft : modulator)
    {
      delta = diff(carrier, fft);
      if (delta < lowest)
      {
        lowest = delta;
        closest = fft;
      }
    }

    return closest;
  }

  void normalize(kiss_fft_scalar tdom[])
  {
    for (int i = 0; i < fft_size; i ++)
      tdom[i] /= fft_size;
  }

  void envelope_hanning(kiss_fft_scalar tdom[])
  {
    for (int i = 0; i < fft_size; i ++)
      tdom[i] *= 0.5*(1 - cos(2*M_PI * i/(double)(fft_size-1)));
  }

  void overlap_add(kiss_fft_scalar output_signal[], const kiss_fft_scalar tdom[], const int offset)
  {
    for (int i = 0; i < fft_size; i ++)
      output_signal[offset+i] += tdom[i];
  }

public:
  TemVoc(const int f, bool a, SndfileHandle c, SndfileHandle m)
    : fft_size{f}, apply_weighting{a}, carrier{c}, modulator{m},
      sample_rate{c.samplerate()}, weighting_envelope{new kiss_fft_scalar[f/2+1]}
  {
    // initialize weighting envelope
    for (int i = 0; i < fft_size/2+1; i ++)
    {
      const double freq {i * sample_rate/(double)fft_size};
      weighting_envelope[i] = apply_weighting ? weighting(freq) : 1.0;
    }
  }

  ~TemVoc()
  {
    delete[] weighting_envelope;
  }

  void process(string output_file, function<void(double)> progress)
  {
    // initialize FFTs
    kiss_fftr_cfg cfg = kiss_fftr_alloc(fft_size, 0, NULL, 0);
    kiss_fftr_cfg cfgi = kiss_fftr_alloc(fft_size, 1, NULL, 0);

    // read modulator
    vector<kiss_fft_cpx *> modulator_fdom;
    kiss_fft_scalar *modulator_signal = new kiss_fft_scalar[modulator.frames()];
    modulator.read(modulator_signal, modulator.frames());
    for (int i = 0; i < modulator.frames() - fft_size; i += fft_size/2)
    {
      kiss_fft_cpx *fdom = new kiss_fft_cpx[fft_size];
      kiss_fftr(cfg, modulator_signal + i, fdom);
      modulator_fdom.push_back(fdom);
    }
    delete[] modulator_signal;

    // read carrier
    kiss_fft_scalar *carrier_signal = new kiss_fft_scalar[carrier.frames()];
    carrier.read(carrier_signal, carrier.frames());

    // initialize output signal
    kiss_fft_scalar *output_signal = new kiss_fft_scalar[carrier.frames() + fft_size];
    for (int i = 0; i < carrier.frames() + fft_size; i ++)
      output_signal[i] = 0.0;

    // process
    kiss_fft_scalar tdom[fft_size];
    kiss_fft_cpx fdom[fft_size];
    for (int i = 0; i < carrier.frames() - fft_size; i += fft_size/2)
    {
      kiss_fftr(cfg, carrier_signal + i, fdom);
      kiss_fft_cpx *closest {find(fdom, modulator_fdom)};
      kiss_fftri(cfgi, closest, tdom);
      normalize(tdom);
      envelope_hanning(tdom);
      overlap_add(output_signal, tdom, i);
      progress(i/(double)carrier.frames());
    }

    for (kiss_fft_cpx *fft : modulator_fdom)
      delete fft;
    delete[] carrier_signal;

    kiss_fft_free(cfg);
    kiss_fft_free(cfgi);

    // save
    SndfileHandle output{output_file, SFM_WRITE,
                         carrier.format(), 1,
                         carrier.samplerate()};
    output.write(output_signal, carrier.frames());
    delete[] output_signal;
  }
};
