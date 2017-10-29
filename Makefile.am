
SUBDIRS = basecvt bat bin2c cpmod dirstack dirtree djinn ftrunc pascii \
     pkgenv ranline stat textlock wrap scripts

AM_CFLAGS = -Wall

pdf:
	mkdir -p doc
	for d in $(SUBDIRS); do \
		for f in $$d/*.1; do \
			man2pdf $$f; \
			mv $$d/*.pdf doc; \
		done \
	done
