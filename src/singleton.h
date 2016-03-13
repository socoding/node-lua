#ifndef SINGLETON_H_
#define SINGLETON_H_
#include "common.h"

/// Should be placed in the appropriate .cpp file somewhere
#define initialise_singleton( type ) \
	template <> type * singleton_t < type > :: m_singleton = 0

/// To be used as a replacement for initialise_singleton( )
///  Creates a file-scoped singleton_t object, to be retrieved with getSingleton
#define create_file_singleton( type ) \
	initialise_singleton( type ); \
	type the##type

#define singleton_ref( type ) (type::getSingleton())
#define singleton_ptr( type ) (type::getSingletonPtr())

template < class type > class singleton_t {
public:
	/// Constructor
	singleton_t( ) {
		/// If you hit this assert, this singleton already exists -- you can't create another one!
		ASSERT( m_singleton == 0 );
		m_singleton = static_cast<type *>(this);
	}

	/// Destructor
	virtual ~singleton_t( ) {
		m_singleton = 0;
	}

	FORCE_INLINE static type & getSingleton( ) { ASSERT( m_singleton ); return *m_singleton; }
	FORCE_INLINE static type * getSingletonPtr( ) { return m_singleton; }

protected:

	/// singleton_t pointer, must be set to 0 prior to creating the object
	static type * m_singleton;
};

#endif
