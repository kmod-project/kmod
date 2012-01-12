MAN5 = depmod.d.5 modprobe.d.5
MAN8 =

dist_man_MANS = $(MAN5) $(MAN8)

EXTRA_DIST = $(MAN5:%.5=%.xml) $(MAN8:%.8=%.xml)
XSLTPROC_FLAGS = \
	--nonet \
	--param funcsynopsis.style "'ansi'"

%.5 %.8: %.xml
	$(AM_V_GEN)$(XSLTPROC) $(XSLTPROC_FLAGS) \
		http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<
