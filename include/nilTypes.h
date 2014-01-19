#pragma once

#include <stdint.h>
#include <exception>
#include <string>
#include <vector>
#include <list>
#include <sstream>
#include <boost/variant.hpp>
#include <boost/algorithm/string.hpp>

namespace nil {

  typedef std::string utf8String;
  typedef std::wstring String;
  typedef uint32_t POVDirection;
  typedef float Real;

  typedef int DeviceID;

  using std::list;
  using std::vector;
  using std::wstringstream;
  using boost::variant;

  //! \struct Vector2i
  //! Two-dimensional integer vector.
  struct Vector2i {
  public:
    int32_t x;
    int32_t y;
    inline Vector2i(): x( 0 ), y( 0 ) {}
    inline explicit Vector2i( int32_t x_, int32_t y_ ): x( x_ ), y( y_ ) {}
    inline bool operator == ( const Vector2i& other ) const
    {
      return ( x == other.x && y == other.y );
    }
    inline bool operator != ( const Vector2i& other ) const
    {
      return ( x != other.x || y != other.y  );
    }
    const static Vector2i ZERO;
  };

  //! \struct Vector3i
  //! Three-dimensional integer vector.
  struct Vector3i {
  public:
    int32_t x;
    int32_t y;
    int32_t z;
    inline Vector3i(): x( 0 ), y( 0 ), z( 0 ) {}
    inline explicit Vector3i( int32_t x_, int32_t y_, int32_t z_ ):
    x( x_ ), y( y_ ), z( z_ ) {}
    inline bool operator == ( const Vector3i& other ) const
    {
      return ( x == other.x && y == other.y && z == other.z );
    }
    inline bool operator != ( const Vector3i& other ) const
    {
      return ( x != other.x || y != other.y || z != other.z  );
    }
    const static Vector3i ZERO;
  };

  //! \struct Vector2f
  //! Two-dimensional floating point vector.
  struct Vector2f {
  public:
    Real x;
    Real y;
    inline Vector2f(): x( 0.0f ), y( 0.0f ) {}
    inline explicit Vector2f( Real x_, Real y_ ): x( x_ ), y( y_ ) {}
    inline bool operator == ( const Vector2f& other ) const
    {
      return ( x == other.x && y == other.y );
    }
    inline bool operator != ( const Vector2f& other ) const
    {
      return ( x != other.x || y != other.y  );
    }
    const static Vector2f ZERO;
  };

  //! \struct Vector3f
  //! Three-dimensional floating point vector.
  struct Vector3f {
  public:
    Real x;
    Real y;
    Real z;
    inline Vector3f(): x( 0.0f ), y( 0.0f ), z( 0.0f ) {}
    inline explicit Vector3f( Real x_, Real y_, Real z_ ):
    x( x_ ), y( y_ ), z( z_ ) {}
    inline bool operator == ( const Vector3f& other ) const
    {
      return ( x == other.x && y == other.y && z == other.z );
    }
    inline bool operator != ( const Vector3f& other ) const
    {
      return ( x != other.x || y != other.y || z != other.z  );
    }
    const static Vector3f ZERO;
  };

}