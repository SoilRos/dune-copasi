#ifndef DUNE_COPASI_TIFF_GRAYSCALE_HH
#define DUNE_COPASI_TIFF_GRAYSCALE_HH

#include <dune/common/exceptions.hh>
#include <dune/common/float_cmp.hh>

#include <tiffio.h>

#include <iostream>
#include <string>

namespace Dune::Copasi {

template<class T>
class TIFFGrayscale
{
  static_assert(std::is_integral_v<T>, "T must be an integral type");
  static_assert(std::is_unsigned_v<T>, "T must be an unsigned type");

  struct TIFFGrayscaleRow
  {
    TIFFGrayscaleRow(const T* const tiff_buffer, const short& col_size)
      : _tiff_buffer(tiff_buffer)
      , _col_size(col_size)
    {}

    double operator[](const T& col) const
    {
      assert((short)col < _col_size);
      return (double)*(_tiff_buffer + col) / std::numeric_limits<T>::max();
    }

    std::size_t size() const { return static_cast<std::size_t>(_col_size); }

    const T* const _tiff_buffer;
    const short& _col_size;
  };

public:
  TIFFGrayscale(const std::string& filename)
    : _tiff_file(TIFFOpen(filename.c_str(), "r"))
  {
    if (not _tiff_file)
      DUNE_THROW(IOError, "Error opening TIFF file '" << filename << "'.");

    short photometric;
    TIFFGetField(_tiff_file, TIFFTAG_PHOTOMETRIC, &photometric);
    if ((photometric != PHOTOMETRIC_MINISBLACK) and
        (photometric != PHOTOMETRIC_MINISBLACK))
      DUNE_THROW(IOError,
                 "TIFF file '" << filename << "' must be in grayscale.");

    short bits_per_sample;
    TIFFGetField(_tiff_file, TIFFTAG_BITSPERSAMPLE, &bits_per_sample);

    if (bits_per_sample != 8 * sizeof(T)) {
      TIFFClose(_tiff_file);
      DUNE_THROW(IOError,
                 "TIFF file '" << filename
                               << "' contains a non-readable grayscale field.");
    }

    TIFFGetField(_tiff_file, TIFFTAG_IMAGELENGTH, &_row_size);
    TIFFGetField(_tiff_file, TIFFTAG_IMAGEWIDTH, &_col_size);
    TIFFGetField(_tiff_file, TIFFTAG_XRESOLUTION, &_x_res);
    TIFFGetField(_tiff_file, TIFFTAG_YRESOLUTION, &_y_res);
    assert(FloatCmp::gt(_x_res, 0.f));
    assert(FloatCmp::gt(_y_res, 0.f));
    _x_off = _y_off = 0.;
    TIFFGetField(_tiff_file, TIFFTAG_XPOSITION, &_x_off);
    TIFFGetField(_tiff_file, TIFFTAG_YPOSITION, &_y_off);
    _tiff_buffer = (T*)_TIFFmalloc(TIFFScanlineSize(_tiff_file));
  }

  TIFFGrayscaleRow operator[](T row) const
  {
    assert((short)row < _row_size);
    // This is not thread safe!!
    TIFFReadScanline(_tiff_file, _tiff_buffer, row);
    return TIFFGrayscaleRow(_tiff_buffer, _col_size);
  }

  template<class DF>
  double operator()(const DF& x, const DF& y)
  {
    // here we assume TIFFTAG_RESOLUTIONUNIT has same dimenssion as grid.
    // since we never check grid units, we also do not check tiff units
    assert(FloatCmp::ge((float)x, _x_off));
    assert(FloatCmp::ge((float)y, _y_off));
    const T i = _x_res * (_x_off + x);
    const T j = _row_size - _y_res * (_y_off + y) - 1;
    return (*this)[j][i];
  }

  std::size_t size() const { return cols(); }

  std::size_t rows() const { return static_cast<std::size_t>(_row_size); }

  std::size_t cols() const { return static_cast<std::size_t>(_col_size); }

  ~TIFFGrayscale()
  {
    if (_tiff_buffer)
      _TIFFfree(_tiff_buffer);
    TIFFClose(_tiff_file);
  }

private:
  TIFF* _tiff_file;
  T* _tiff_buffer;
  short _row_size;
  short _col_size;
  float _x_res, _x_off;
  float _y_res, _y_off;
};

} // namespace Dune::Copasi

#endif // DUNE_COPASI_TIFF_GRAYSCALE_HH