#include <com/sun/star/io/XOutputStream.hpp>
#include <sal/log.hxx>
#include <com/sun/star/embed/XRelationshipAccess.hpp>
#include <com/sun/star/io/BufferSizeExceededException.hpp>
#include <com/sun/star/io/NotConnectedException.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/lang/XEventListener.hpp>
#include <cppuhelper/queryinterface.hxx>
#include <sal/types.h>
#include <vector>
#include <comphelper/vecstream.hxx>

namespace comphelper
{
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::io;
using namespace ::com::sun::star::uno;
using namespace ::osl;

// ----------- VectorInputStream -----------
VectorInputStream::VectorInputStream(std::shared_ptr<std::vector<sal_Int8>> vec)
    : m_vec(vec)
    , m_pos(0)
{
}

sal_Int32 SAL_CALL VectorInputStream::available() { return m_vec->size() - m_pos; }

void SAL_CALL VectorInputStream::closeInput() {}

sal_Int32 SAL_CALL VectorInputStream::readBytes(css::uno::Sequence<sal_Int8>& data, sal_Int32 count)
{
    if (count < 0)
        throw BufferSizeExceededException(OUString(), *this);

    if (count == 0 || available() == 0)
        return 0;

    std::scoped_lock gaurd(m_mutex);

    sal_Int32 avail = m_vec->size() - m_pos;

    if (avail < count)
        count = avail;

    data.realloc(count);
    memcpy(data.getArray(), m_vec->data() + m_pos, count);
    m_pos += count;

    return count;
}
sal_Int32 VectorInputStream::readSomeBytes(sal_Int8* data, sal_Int32 count)
{
    if (count < 0)
        throw BufferSizeExceededException(OUString(), *this);

    std::scoped_lock gaurd(m_mutex);
    sal_Int32 avail = m_vec->size() - m_pos;

    if (avail < count)
        count = avail;

    memcpy(data, m_vec->data() + m_pos, count);
    m_pos += count;

    return count;
}

sal_Int32 SAL_CALL VectorInputStream::readSomeBytes(Sequence<sal_Int8>& data, sal_Int32 count)
{
    return readBytes(data, count);
}

void SAL_CALL VectorInputStream::skipBytes(sal_Int32 skip)
{
    if (skip < 0)
        throw BufferSizeExceededException(OUString(), *this);

    std::scoped_lock aGuard(m_mutex);

    sal_Int32 avail = m_vec->size() - m_pos;

    if (avail < skip)
        skip = avail;

    m_pos += skip;
}

void SAL_CALL VectorInputStream::seek(sal_Int64 location)
{
    if (location > (sal_Int64)m_vec->size() || location < 0 || location > SAL_MAX_INT32)
        throw IllegalArgumentException("bad location", static_cast<cppu::OWeakObject*>(this), 1);
    std::scoped_lock gaurd(m_mutex);
    m_pos = static_cast<sal_Int32>(location);
}

sal_Int64 SAL_CALL VectorInputStream::getPosition()
{
    std::scoped_lock gaurd(m_mutex);
    return m_pos;
}

sal_Int64 SAL_CALL VectorInputStream::getLength()
{
    std::scoped_lock gaurd(m_mutex);
    return m_vec->size();
}
Any SAL_CALL VectorInputStream::queryInterface(const Type& rType)
{
    Any aRet = cppu::queryInterface(rType, static_cast<embed::XRelationshipAccess*>(this),
                                    static_cast<lang::XTypeProvider*>(this),
                                    static_cast<io::XInputStream*>(this));
    if (aRet.hasValue())
        return aRet;

    return OWeakObject::queryInterface(rType);
}
Sequence<Type> SAL_CALL VectorInputStream::getTypes()
{
    static css::uno::Sequence<css::uno::Type> aTypes = {
        cppu::UnoType<css::lang::XTypeProvider>::get(),
        cppu::UnoType<embed::XRelationshipAccess>::get(),
        cppu::UnoType<io::XInputStream>::get(),
    };
    return aTypes;
}

Sequence<sal_Int8> SAL_CALL VectorInputStream::getImplementationId()
{
    return Sequence<sal_Int8>();
}

void SAL_CALL VectorInputStream::acquire() noexcept { OWeakObject::acquire(); }

void SAL_CALL VectorInputStream::release() noexcept { OWeakObject::release(); }

// ----------- VectorOutputStream -----------

VectorOutputStream::VectorOutputStream(std::shared_ptr<std::vector<sal_Int8>> vec)
    : m_vec(vec)
    , m_pos(0)
{
}

void SAL_CALL VectorOutputStream::writeBytes(const Sequence<sal_Int8>& data)
{
    std::scoped_lock gaurd(m_mutex);
    sal_Int32 available = m_vec->size() - m_pos;
    if (available < data.getLength())
    {
        std::size_t newSize = static_cast<std::size_t>(m_pos + data.getLength());
        m_vec->resize(newSize);
    }
    memcpy(m_vec->data() + m_pos, data.getConstArray(), data.getLength());
    m_pos += data.getLength();
}

void SAL_CALL VectorOutputStream::flush()
{
    // if the vector is the right size, this is a no-op, if it's writing over an existing stream, it gets truncated to end of the last write
    m_vec->resize(m_pos);
}

void SAL_CALL VectorOutputStream::closeOutput()
{
    // see ::flush() for why
    m_vec->resize(m_pos);
}

Any SAL_CALL VectorOutputStream::queryInterface(const Type& rType)
{
    Any aRet = cppu::queryInterface(rType, static_cast<embed::XRelationshipAccess*>(this),
                                    static_cast<lang::XTypeProvider*>(this),
                                    static_cast<io::XOutputStream*>(this));
    if (aRet.hasValue())
        return aRet;

    return OWeakObject::queryInterface(rType);
}
Sequence<Type> SAL_CALL VectorOutputStream::getTypes()
{
    static css::uno::Sequence<css::uno::Type> aTypes = {
        cppu::UnoType<css::lang::XTypeProvider>::get(),
        cppu::UnoType<embed::XRelationshipAccess>::get(),
        cppu::UnoType<io::XOutputStream>::get(),
    };
    return aTypes;
}

Sequence<sal_Int8> SAL_CALL VectorOutputStream::getImplementationId()
{
    return Sequence<sal_Int8>();
}

void SAL_CALL VectorOutputStream::acquire() noexcept { OWeakObject::acquire(); }

void SAL_CALL VectorOutputStream::release() noexcept { OWeakObject::release(); }

VecStreamContainer::VecStreamContainer(Reference<VecStreamSupplier>& stream)
    : m_stream(stream)
{
}

// ----------- VecStreamSupplier -----------

VecStreamSupplier::VecStreamSupplier(Reference<VectorInputStream> inputStream,
                                     Reference<VectorOutputStream> outputStream)
    : m_inputStream(std::move(inputStream))
    , m_outputStream(std::move(outputStream))
{
    m_seekable.set(m_inputStream, uno::UNO_QUERY);
    if (!m_seekable.is())
        m_seekable.set(m_outputStream, uno::UNO_QUERY);
}

Reference<io::XInputStream> SAL_CALL VecStreamSupplier::getInputStream() { return m_inputStream; }

Reference<io::XOutputStream> SAL_CALL VecStreamSupplier::getOutputStream()
{
    return m_outputStream;
}

void SAL_CALL VecStreamSupplier::seek(sal_Int64 location)
{
    if (!m_seekable.is())
        throw io::NotConnectedException();
    m_seekable->seek(location);
}

sal_Int64 SAL_CALL VecStreamSupplier::getPosition()
{
    if (!m_seekable.is())
        throw io::NotConnectedException();
    return m_seekable->getPosition();
}

sal_Int64 SAL_CALL VecStreamSupplier::getLength()
{
    if (!m_seekable.is())
        throw io::NotConnectedException();

    return m_seekable->getLength();
}

Any SAL_CALL VecStreamSupplier::queryInterface(const Type& rType)
{
    Any aRet
        = cppu::queryInterface(rType, static_cast<embed::XRelationshipAccess*>(this),
                               static_cast<lang::XTypeProvider*>(this),
                               static_cast<io::XStream*>(this), static_cast<io::XSeekable*>(this));
    if (aRet.hasValue())
        return aRet;

    return OWeakObject::queryInterface(rType);
}
Sequence<Type> SAL_CALL VecStreamSupplier::getTypes()
{
    static css::uno::Sequence<css::uno::Type> aTypes = {
        cppu::UnoType<css::lang::XTypeProvider>::get(),
        cppu::UnoType<embed::XRelationshipAccess>::get(),
        cppu::UnoType<io::XStream>::get(),
        cppu::UnoType<io::XSeekable>::get(),
    };
    return aTypes;
}

Sequence<sal_Int8> SAL_CALL VecStreamSupplier::getImplementationId()
{
    return Sequence<sal_Int8>();
}

void SAL_CALL VecStreamSupplier::acquire() noexcept { OWeakObject::acquire(); }

void SAL_CALL VecStreamSupplier::release() noexcept { OWeakObject::release(); }

// ----------- VecStreamContainer -----------
Reference<io::XInputStream> SAL_CALL VecStreamContainer::getInputStream()
{
    return m_stream->getInputStream();
}
Reference<io::XOutputStream> SAL_CALL VecStreamContainer::getOutputStream()
{
    return m_stream->getOutputStream();
}

Any SAL_CALL VecStreamContainer::queryInterface(const Type& rType)
{
    Any aRet = cppu::queryInterface(rType, static_cast<embed::XExtendedStorageStream*>(this),
                                    static_cast<io::XStream*>(this),
                                    static_cast<embed::XRelationshipAccess*>(this));
    if (aRet.hasValue())
        return aRet;

    return OWeakObject::queryInterface(rType);
}

void SAL_CALL VecStreamContainer::acquire() noexcept { OWeakObject::acquire(); }

void SAL_CALL VecStreamContainer::release() noexcept { OWeakObject::release(); }

void SAL_CALL VecStreamContainer::dispose()
{
    std::unique_lock gaurd(m_mutex);
    if (m_listeners.getLength(gaurd))
    {
        lang::EventObject aSource(getXWeak());
        m_listeners.disposeAndClear(gaurd, aSource);
    }
}

void SAL_CALL VecStreamContainer::addEventListener(const Reference<lang::XEventListener>& xListener)
{
    std::unique_lock gaurd(m_mutex);

    m_listeners.addInterface(gaurd, xListener);
}

void SAL_CALL
VecStreamContainer::removeEventListener(const Reference<lang::XEventListener>& listener)
{
    std::unique_lock gaurd(m_mutex);

    m_listeners.removeInterface(gaurd, listener);
}

}
