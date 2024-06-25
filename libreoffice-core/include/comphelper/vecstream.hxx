#include <config_options.h>
#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/io/XInputStream.hpp>
#include <com/sun/star/io/XOutputStream.hpp>
#include <com/sun/star/io/XSeekable.hpp>
#include <cppuhelper/implbase.hxx>
#include <comphelper/comphelperdllapi.h>
#include <comphelper/bytereader.hxx>
#include <mutex>

namespace comphelper
{

class COMPHELPER_DLLPUBLIC VectorInputStream final
    : public ::cppu::WeakImplHelper< css::io::XInputStream, css::io::XSeekable >,
      public comphelper::ByteReader
{
    std::vector<sal_Int8>& m_vec;
    std::mutex m_mutex;
    sal_Int32 m_pos;

public:
    VectorInputStream(std::vector<sal_Int8>& vec);
    virtual sal_Int32 SAL_CALL readBytes( css::uno::Sequence<sal_Int8>& aData, sal_Int32 nBytesToRead ) override;

    virtual sal_Int32 SAL_CALL readSomeBytes( css::uno::Sequence<sal_Int8>& aData, sal_Int32 nMaxBytesToRead ) override;

    virtual void SAL_CALL skipBytes( sal_Int32 nBytesToSkip ) override;

    virtual sal_Int32 SAL_CALL available(  ) override;

    virtual void SAL_CALL closeInput(  ) override;

    virtual void SAL_CALL seek( sal_Int64 location ) override;
    virtual sal_Int64 SAL_CALL getPosition(  ) override;
    virtual sal_Int64 SAL_CALL getLength(  ) override;

    virtual sal_Int32 readSomeBytes( sal_Int8* pData, sal_Int32 nBytesToRead ) override;
};

class COMPHELPER_DLLPUBLIC VectorOutputStream final
    : public ::cppu::WeakImplHelper< css::io::XOutputStream >
{
    std::vector<sal_Int8>& m_vec;
    sal_Int32 m_pos;

    std::mutex m_mutex;

public:
    VectorOutputStream(std::vector<sal_Int8>& vec);
    virtual void SAL_CALL writeBytes( const css::uno::Sequence< sal_Int8 >& aData ) override;
    virtual void SAL_CALL flush(  ) override;
    virtual void SAL_CALL closeOutput(  ) override;
};
}
