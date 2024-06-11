MAN5 = depmod.d.5 modprobe.d.5 modules.dep.5
MAN8 = kmod.8 depmod.8 insmod.8 lsmod.8 rmmod.8 modprobe.8 modinfo.8
MAN_STUB = modules.dep.bin.5

AM_V_SCDOC = $(AM_V_SCDOC_$(V))
AM_V_SCDOC_ = $(AM_V_SCDOC_$(AM_DEFAULT_VERBOSITY))
AM_V_SCDOC_0 = @echo "  SCDOC    " $@;

if BUILD_TOOLS
dist_man_MANS = $(MAN5) $(MAN8) $(MAN_STUB)
endif

EXTRA_DIST = $(MAN5:%.5=%.5.scd) $(MAN8:%.8=%.8.scd)
CLEANFILES = $(filter-out $(MAN_STUB), $(dist_man_MANS))

define generate_manpage
	$(AM_V_SCDOC)if [ '$(distconfdir)' != '/lib' ] ; then \
		sed -e 's|@DISTCONFDIR@|$(distconfdir)|g' $< ; \
	else \
		sed -e '/@DISTCONFDIR@/d' $< ; \
	fi | \
	sed -e 's|@MODULE_DIRECTORY@|$(module_directory)|g' | \
	$(SCDOC) > $@
endef

%.5: %.5.scd
	$(generate_manpage)

%.8: %.8.scd
	$(generate_manpage)
