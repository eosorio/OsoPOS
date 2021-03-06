# rpmrebuild autogenerated specfile

BuildRoot: /home/eosorio/.tmp/rpmrebuild.6866/work/root
AutoProv: no
%undefine __find_provides
AutoReq: no
%undefine __find_requires
# Do not try autogenerate prereq/conflicts/obsoletes and check files
%undefine __check_files
%undefine __find_prereq
%undefine __find_conflicts
%undefine __find_obsoletes
# Be sure buildpolicy set to do nothing
%define __spec_install_post %{nil}
# Something that need for rpm-4.1
%define _missing_doc_files_terminate_build 0
#dummy
#dummy
#BUILDHOST:    matarile
#BUILDTIME:    Thu Feb  8 19:37:28 2001
#SOURCERPM:    osopos-2.0-alpha.src.rpm

#RPMVERSION:   4.0
#COOKIE:       matarile 981682648


#OS:           Linux
#SIZE:           438128
#ARCHIVESIZE:           440108
#ARCH:         i386
BuildArch:     i386
Name:          osopos
Version:       2.0
Release:       alpha
License:       GPL 
Group:         Applications/Office
Summary:       Un punto de venta para el pequeño y mediano comercio
Distribution:  LINUCS

URL:           http://punto-deventa.com
Vendor:        LINUCS
Packager:      Eduardo Israel Osorio Hernández <iosorio@punto-deventa.com>





Provides:      osopos = 2.0-alpha
Requires:      ncurses >= 5
Requires:      postgresql >= 7
Requires:      /bin/sh  
#Requires:      rpmlib(PayloadFilesHavePrefix) <= 4.0-1
Requires:      ld-linux.so.2  
Requires:      libc.so.6  
Requires:      libcrypt.so.1  
Requires:      libform.so.5  
Requires:      libncurses.so.5  
Requires:      libpq.so.2.1  
Requires:      /bin/bash  
Requires:      libc.so.6(GLIBC_2.0)  
Requires:      libc.so.6(GLIBC_2.1)  
Requires:      libc.so.6(GLIBC_2.1.3)  
#Requires:      rpmlib(CompressedFileNames) <= 3.0.4-1
#suggest
#enhance
%description
OsoPOS es un programa para convertir una computadora en un potente punto de
venta, reemplazando a las cajas registradoras. El sistema consta de un
programa de facturación, caja, inventarios, corte de caja y un impresor de
comprobantes fiscales.
%files
%dir %attr(0750, scaja, osopos) "/home/scaja/.osopos"
%config %attr(0750, scaja, osopos) "/home/scaja/.osopos/factur.config"
%config %attr(0750, scaja, osopos) "/home/scaja/.osopos/imprrem.config"
%config %attr(0750, scaja, osopos) "/home/scaja/.osopos/invent.config"
%config %attr(0750, scaja, osopos) "/home/scaja/.osopos/remision.config"
%attr(0700, scaja, osopos) "/usr/bin/corte"
%attr(0750, scaja, osopos) "/usr/bin/factur"
%attr(0750, scaja, osopos) "/usr/bin/imprem"
%attr(0750, scaja, osopos) "/usr/bin/imprrem"
%attr(0700, scaja, osopos) "/usr/bin/invent"
%attr(0700, scaja, osopos) "/usr/bin/inventario"
%attr(0750, scaja, osopos) "/usr/bin/precio"
%attr(0750, scaja, osopos) "/usr/bin/remision"
%attr(0700, scaja, osopos) "/usr/bin/respalda"
%attr(0700, scaja, osopos) "/usr/bin/restaura"
%attr(0750, scaja, osopos) "/usr/bin/venta"
%doc %attr(0750, scaja, osopos) "/usr/share/doc/osopos/BUGS"
%doc %attr(0750, scaja, osopos) "/usr/share/doc/osopos/COPYING"
%doc %attr(0750, scaja, osopos) "/usr/share/doc/osopos/FEATURES"
%doc %attr(0750, scaja, osopos) "/usr/share/doc/osopos/INSTALL"
%doc %attr(0750, scaja, osopos) "/usr/share/doc/osopos/LEAME"
%doc %attr(0750, scaja, osopos) "/usr/share/doc/osopos/LICENCIA"
%doc %attr(0750, scaja, osopos) "/usr/share/doc/osopos/README"
%doc %attr(0750, scaja, osopos) "/usr/share/doc/osopos/TODO"
%doc %attr(0750, scaja, osopos) "/usr/share/doc/osopos/manual_inventario.txt"
%doc %attr(0750, scaja, osopos) "/usr/share/doc/osopos/manual_remision.txt"
%doc %attr(0700, scaja, root) "/usr/share/doc/osopos/tablas.sql"
%dir %attr(0750, scaja, osopos) "/var/log/osopos"
%pre -p /bin/sh
echo "Creando usuarios de OsoPOS..."
/usr/sbin/groupadd osopos
/usr/sbin/useradd -g osopos scaja
/usr/sbin/useradd -g osopos cajero1
%post -p /bin/sh
echo "Creando base de datos..."
/bin/su -c "/usr/bin/createuser -d -a scaja" --login postgres
/bin/su -c "/usr/bin/createuser -D -A cajero1" --login postgres
/bin/su -c "/usr/bin/createdb osopos" --login scaja
/bin/su -c "/usr/bin/psql osopos < /usr/share/doc/osopos/tablas.sql 2>/var/log/osopos/createdb_errors"  --login scaja
%preun -p /bin/sh
if [ -f /var/log/osopos/createdb_errors ]; then
  rm -rf /var/log/osopos/createdb_errors
fi
%postun -p /bin/sh
if [ -f /tmp/osopos_db.sql ]; then
  rm -rf /tmp/osopos_db.sql
fi
/bin/su -c "/usr/bin/pg_dump osopos > /tmp/osopos_db.sql" --login scaja
echo "A punto de borrar la base de datos. Se ha creado un respaldo en /tmp/osopos_db.sql"
/bin/su -c "/usr/bin/dropuser cajero1" --login postgres
/bin/su -c "/usr/bin/dropdb osopos" --login postgres
/bin/su -c "/usr/bin/dropuser scaja" --login postgres
/usr/sbin/userdel cajero1
/usr/sbin/userdel scaja
/usr/sbin/groupdel osopos
%changelog
