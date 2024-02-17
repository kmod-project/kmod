MAN5 = depmod.d.5 modprobe.d.5 modules.dep.5
MAN8 = kmod.8 depmod.8 insmod.8 lsmod.8 rmmod.8 modprobe.8 modinfo.8
MAN_STUB = modules.dep.bin.5

AM_V_XSLT = $(AM_V_XSLT_$(V))
AM_V_XSLT_ = $(AM_V_XSLT_$(AM_DEFAULT_VERBOSITY))
AM_V_XSLT_0 = @echo "  XSLT    " $@;

XSLT = $(if $(XSLTPROC), $(XSLTPROC), xsltproc)

if BUILD_TOOLS
dist_man_MANS = $(MAN5) $(MAN8) $(MAN_STUB)
modules.dep.bin.5: modules.dep.5
endif

EXTRA_DIST = $(MAN5:%.5=%.5.xml) $(MAN8:%.8=%.8.xml)
CLEANFILES = $(dist_man_MANS)

define generate_manpage
	$(AM_V_XSLT)if [ '$(distconfdir)' != '/lib' ] ; then \
		sed -e 's|@DISTCONFDIR@|$(distconfdir)|g' $< ; \
	else \
		sed -e '/@DISTCONFDIR@/d' $< ; \
	fi | \
	sed -e 's|@MODULE_DIRECTORY@|$(module_directory)|g' | \
	$(XSLT) \
		-o $@ \
		--nonet \
		--stringparam man.output.quietly 1 \
		--param funcsynopsis.style "'ansi'" \
		http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl -
endef

%.5: %.5.xml
	$(generate_manpage)

%.8: %.8.xml
	$(generate_manpage)
