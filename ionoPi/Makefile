#
# ionoPi
#
#     Copyright (C) 2016-2017 Sfera Labs S.r.l.
#
#     For information, see the Iono Pi web site:
#     http://www.sferalabs.cc/iono-pi
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Lesser Public License for more details.
#
# You should have received a copy of the GNU General Lesser Public
# License along with this program.  If not, see
# <http://www.gnu.org/licenses/lgpl-3.0.html>.
#

 

INSTALL_DIR = /usr/local

LIB_NAME = ionoPi
LIB = lib$(LIB_NAME).so
UTILITY = iono

HEADERS = ionoPi.h
LIB_OBJ = ionoPi.o
UTILITY_OBJ = ionoPiUtil.o

CC = gcc
CFLAGS = -Wall -fPIC -I.

# utility recompiled when object files or library modified
$(UTILITY) : $(UTILITY_OBJ) $(LIB)
	@ echo "Linking $@ utility ..."
	@ $(CC) -o $@ $(LIB) $(UTILITY_OBJ)

# library recompiled when object files modified
$(LIB) : $(LIB_OBJ)
	@ echo "Linking shared lib ..."
	@ $(CC) -shared -Wl,-soname,$@ -o $@ -lwiringPi -lwiringPiDev -lpthread $(LIB_OBJ)

# object files recompiled when source files modified (.c and .h)
%.o : %.c $(HEADERS)
	@ echo "Compiling $< ..."
	@ $(CC) -c -o $@ $< $(CFLAGS)

# install utility and lib
.PHONY:	install
install : $(UTILITY) install-lib
	@ echo "Installing $(UTILITY) utility ..."
	@ cp $(UTILITY) $(INSTALL_DIR)/bin
	@ chown root:root	$(INSTALL_DIR)/bin/$(UTILITY)
	@ chmod 4755		$(INSTALL_DIR)/bin/$(UTILITY)

# install only lib 
.PHONY:	install-lib
install-lib : $(LIB)
	@ echo "Installing lib ..."
	@ install -m 0755 -d $(INSTALL_DIR)/include
	@ install -m 0644 $(HEADERS) $(INSTALL_DIR)/include
	@ install -m 0755 -d $(INSTALL_DIR)/lib
	@ install -m 0755 $(LIB) $(INSTALL_DIR)/lib
	@ ldconfig
	
.PHONY:	uninstall
uninstall:
	@ echo "Uninstalling lib ..."
	@ cd $(INSTALL_DIR)/include/ && rm -f $(HEADERS)
	@ cd $(INSTALL_DIR)/lib/     && rm -f $(LIB)
	@ ldconfig
	@ echo "Uninstalling $(UTILITY) utility ..."
	@ rm -f $(INSTALL_DIR)/bin/$(UTILITY)

.PHONY: clean
clean:
	@ echo "Cleaning ..."
	@ rm -f $(LIB) $(UTILITY) *.o *~ core
 