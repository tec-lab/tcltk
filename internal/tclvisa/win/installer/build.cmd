@set TCLVISA_PLATFORM=x86
@erase tclvisa.wixobj
@candle tclvisa.wxs -ext WixUIExtension
@light tclvisa.wixobj -ext WixUIExtension -out tclvisa-0.3.0-win-x86.msi

@set TCLVISA_PLATFORM=x64
@erase tclvisa.wixobj
@candle tclvisa.wxs -ext WixUIExtension
@light tclvisa.wixobj -ext WixUIExtension -out tclvisa-0.3.0-win-x64.msi

