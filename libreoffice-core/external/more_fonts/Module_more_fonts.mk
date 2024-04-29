# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Module_Module,more_fonts))

# MACRO: Only keep fonts that support metric-compatible replacement {

# # For Hebrew support:
# 	ExternalPackage_alef \
# 	UnpackedTarball_alef \
# 	ExternalPackage_libre_hebrew \
# 	UnpackedTarball_libre_hebrew \
# # For Arabic support:
# 	UnpackedTarball_amiri \
# 	ExternalPackage_amiri \
# 	UnpackedTarball_scheherazade \
# 	ExternalPackage_scheherazade \
# 	UnpackedTarball_reem \
# 	ExternalPackage_reem \

# # ???
# 	ExternalPackage_gentium \
# 	UnpackedTarball_gentium \
# # Dyslexia
# 	ExternalPackage_opendyslexic \
# 	UnpackedTarball_opendyslexic \

# # Source code
# 	ExternalPackage_sourcesans \
# 	UnpackedTarball_sourcesans \

# # Just a nice font I guess?
# 	UnpackedTarball_karla \
# 	ExternalPackage_karla \
# 	ExternalPackage_dejavu \
# 	UnpackedTarball_dejavu \
# 	UnpackedTarball_opensans \
# 	ExternalPackage_opensans \

# # Times New Roman-esque but Liberation already does that
# 	UnpackedTarball_libertineg \
# 	ExternalPackage_libertineg \

# MACRO: }

$(eval $(call gb_Module_add_targets,more_fonts,\
	ExternalPackage_caladea \
	ExternalPackage_carlito \
	ExternalPackage_liberation \
	ExternalPackage_liberation_narrow \
	ExternalPackage_noto_sans \
	ExternalPackage_noto_serif \
	UnpackedTarball_caladea \
	UnpackedTarball_carlito \
	UnpackedTarball_liberation \
	UnpackedTarball_liberation_narrow \
	UnpackedTarball_noto_sans \
	UnpackedTarball_noto_serif \
))

# vim: set noet sw=4 ts=4:
