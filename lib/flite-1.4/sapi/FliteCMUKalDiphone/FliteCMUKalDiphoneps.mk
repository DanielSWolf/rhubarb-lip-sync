
FliteCMUKalDiphoneps.dll: dlldata.obj FliteCMUKalDiphone_p.obj FliteCMUKalDiphone_i.obj
	link /dll /out:FliteCMUKalDiphoneps.dll /def:FliteCMUKalDiphoneps.def /entry:DllMain dlldata.obj FliteCMUKalDiphone_p.obj FliteCMUKalDiphone_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del FliteCMUKalDiphoneps.dll
	@del FliteCMUKalDiphoneps.lib
	@del FliteCMUKalDiphoneps.exp
	@del dlldata.obj
	@del FliteCMUKalDiphone_p.obj
	@del FliteCMUKalDiphone_i.obj
