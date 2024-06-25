#include "com/sun/star/io/BufferSizeExceededException.hdl"
#include "com/sun/star/lang/IllegalArgumentException.hdl"
#include "sal/log.hxx"
#include "sal/types.h"
#include <vector>
#include <comphelper/vecstream.hxx>

namespace comphelper
{
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::io;
using namespace ::com::sun::star::uno;
using namespace ::osl;


VectorInputStream::VectorInputStream(std::vector<sal_Int8>& vec)
    : m_vec(vec)
    , m_pos(0){}


sal_Int32 SAL_CALL VectorInputStream::available()
{
    return m_vec.size() - m_pos;
}

void SAL_CALL VectorInputStream::closeInput(){}


sal_Int32 SAL_CALL VectorInputStream::readBytes(css::uno::Sequence<sal_Int8>& data, sal_Int32 count)
{
    SAL_WARN("vecstream", "readBytes " << count);
    if (count < 0)
        throw BufferSizeExceededException(OUString(),*this);

    if (count == 0 || available() == 0)
        return 0;

    std::scoped_lock gaurd(m_mutex);

    sal_Int32 avail = m_vec.size() - m_pos;

    if (avail < count)
        count = avail;

    data.realloc(count);
    memcpy(data.getArray(), m_vec.data() + m_pos, count);
    m_pos += count;

    return count;
}
sal_Int32 VectorInputStream::readSomeBytes( sal_Int8* data, sal_Int32 count )
{
    if (count < 0)
        throw BufferSizeExceededException(OUString(),*this);

    std::scoped_lock gaurd( m_mutex );
    sal_Int32 avail = m_vec.size() - m_pos;

    if (avail < count)
        count = avail;

    memcpy(data, m_vec.data() + m_pos, count);
    m_pos += count;

    return count;
}

sal_Int32 SAL_CALL VectorInputStream::readSomeBytes( Sequence<sal_Int8>& data, sal_Int32 count )
{
    return readBytes(data, count);
}

void SAL_CALL VectorInputStream::skipBytes( sal_Int32 skip )
{
    if (skip < 0)
        throw BufferSizeExceededException(OUString(),*this);

    std::scoped_lock aGuard( m_mutex );

    sal_Int32 avail = m_vec.size() - m_pos;

    if (avail < skip)
        skip = avail;

    m_pos += skip;
}

void SAL_CALL VectorInputStream::seek( sal_Int64 location )
{
    if ( location > m_vec.size() || location < 0 || location > SAL_MAX_INT32 )
        throw IllegalArgumentException("bad location", static_cast<cppu::OWeakObject*>(this), 1);
    std::scoped_lock gaurd( m_mutex );
    m_pos = static_cast<sal_Int32>(location);
}

sal_Int64 SAL_CALL VectorInputStream::getPosition()
{
    std::scoped_lock gaurd( m_mutex );
    return m_pos;
}

sal_Int64 SAL_CALL VectorInputStream::getLength(  )
{
    std::scoped_lock gaurd(m_mutex);
    return m_vec.size();
}

VectorOutputStream::VectorOutputStream(std::vector<sal_Int8>& vec)
    : m_vec(vec)
    , m_pos(0){}


void SAL_CALL VectorOutputStream::writeBytes( const Sequence< sal_Int8 >& data )
{
    std::scoped_lock gaurd(m_mutex);
    sal_Int32 available = m_vec.size() - m_pos;
    if ( available < data.getLength())
    {

        std::size_t newSize = static_cast<std::size_t>(m_pos + data.getLength());
        m_vec.resize(newSize);
    }
    std::copy(data.getConstArray(), data.getConstArray() + data.getLength(), m_vec.data() + m_pos);
    m_pos += data.getLength();
}

void SAL_CALL VectorOutputStream::flush(){}
void SAL_CALL VectorOutputStream::closeOutput(){}
}
