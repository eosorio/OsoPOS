CREATE TABLE articulos (
  codigo        varchar(20) PRIMARY KEY NOT NULL,
  descripcion   varchar(50) NOT NULL,
  pu            real,
  descuento     real DEFAULT 0,
  cant          real,
  empaque       real,
  medida        varchar(5),
  min           int DEFAULT 0,
  max           int,
  id_prov1      int4 DEFAULT 0,
  id_prov2      int4 DEFAULT 0,
  id_depto      int4 DEFAULT 0,
  p_costo       real DEFAULT 0,
  prov_clave    varchar(20) DEFAULT '',
  iva_porc      int DEFAULT 15 NOT NULL,
  divisa        character(3) NOT NULL DEFAULT 'MXP',
  codigo2       varchar(20),
  pu2           real,
  pu3           real,
  pu4           real,
  pu5           real,
  tax_0         real DEFAULT 5,
  tax_1         real DEFAULT 0,
  tax_2         real DEFAULT 0,
  tax_3         real DEFAULT 0,
  tax_4         real DEFAULT 0,
  tax_5         real DEFAULT 0
);
REVOKE ALL ON articulos FROM PUBLIC;
GRANT SELECT ON articulos to GROUP osopos;
GRANT ALL ON articulos TO "scaja";

CREATE TABLE articulos_costos (
  codigo        varchar(20) NOT NULL,
  id_prov       int4 NOT NULL,
  costo1        real NOT NULL,
  costo2        real NOT NULL,
  prov_clave    varchar(20),
  iva_porc      real NOT NULL,
  entrega1      int2,
  entrega2      int2,
  costo_envio1  real,
  costo_envio2  real,
  actualizacion datetime,
  status        character(1),
  divisa        character(3) NOT NULL DEFAULT 'MXP'
);
REVOKE ALL ON articulos_costos FROM PUBLIC;
GRANT SELECT ON articulos_costos to GROUP osopos;
GRANT ALL ON articulos_costos TO "scaja";
  

);

CREATE TABLE almacen_1 (
  codigo        varchar(20) PRIMARY KEY NOT NULL,
  pu            real DEFAULT 0,
  cant          real DEFAULT 0,
  medida        varchar(5),
  c_min         real DEFAULT 0,
  c_max         real,
  divisa        character(3) NOT NULL DEFAULT 'MXP',
  codigo2       varchar(20),
  pu2           real,
  pu3           real,
  pu4           real,
  pu5           real,
  tax_0         real DEFAULT 5,
  tax_1         real DEFAULT 0,
  tax_2         real DEFAULT 0,
  tax_3         real DEFAULT 0,
  tax_4         real DEFAULT 0,
  tax_5         real DEFAULT 0
);
REVOKE ALL ON almacen_1 FROM PUBLIC;
GRANT SELECT ON almacen_1 to GROUP osopos;
GRANT ALL ON almacen_1 TO "scaja";

CREATE TABLE "almacenes" (
  "id"          SERIAL,
  "nombre"      VARCHAR(30),
  "descripcion" VARCHAR(40)
);
REVOKE ALL ON almacenes FROM PUBLIC;
GRANT SELECT ON almacenes to GROUP osopos;
GRANT ALL ON almacenes TO scaja;

CREATE TABLE "article_desc" (
    "code"          character varying(20) NOT NULL,
    "descripcion"   character varying(50) NOT NULL DEFAULT '',
    "long_desc"     character varying(1000),
    "id_prov1"      int4 DEFAULT 0,
    "id_prov2"      int4 DEFAULT 0,
    "id_prov3"      int4 DEFAULT 0,
    "prov_clave1"   varchar(20) DEFAULT '',
    "prov_clave2"   varchar(20) DEFAULT '',
    "prov_clave3"   varchar(20) DEFAULT '',
    "img_location"  character varying(45),
    "url"           character varying(80),
    "p_costo"       real DEFAULT 0,
    "p_promedio"    real,
    "notes"         character varying(500),
    Constraint "article_desc_pkey" Primary Key ("code")
);
REVOKE ALL ON article_desc FROM PUBLIC;
GRANT SELECT ON article_desc to GROUP osopos;
GRANT ALL ON article_desc TO "scaja";

CREATE TABLE compras (
  codigo        varchar(20) NOT NULL,
  ct            real NOT NULL,
  cookie        varchar(8) NOT NULL,
  costo         real NOT NULL,
  Primary Key ("codigo")
);
REVOKE ALL ON compras FROM PUBLIC;
GRANT SELECT ON compras to GROUP osopos;
GRANT ALL ON compras TO "scaja";

CREATE TABLE corte (
  numero        int4 PRIMARY KEY NOT NULL,
  bandera       BIT(8) DEFAULT B'0' NOT NULL
);
REVOKE ALL ON corte FROM PUBLIC;
GRANT SELECT ON corte to GROUP osopos;
GRANT INSERT,SELECT,UPDATE ON corte TO "supervisor";

CREATE TABLE ventas (
  numero            SERIAL PRIMARY KEY,
  monto             real,
  tipo_pago         int2 DEFAULT 20 NOT NULL,
  tipo_factur       int2 DEFAULT 5 NOT NULL,
  corte             BIT(8) DEFAULT B'00000000',
  utilidad          real,
  id_vendedor       int4 DEFAULT 0 NOT NULL,
  id_cajero         int4 DEFAULT 0 NOT NULL,
  fecha             date NOT NULL,
  hora              time NOT NULL,
  iva               real NOT NULL,
  tax_0             real DEFAULT 0 NOT NULL,
  tax_1             real DEFAULT 0 NOT NULL,
  tax_2             real DEFAULT 0 NOT NULL,
  tax_3             real DEFAULT 0 NOT NULL,
  tax_4             real DEFAULT 0 NOT NULL,
  tax_5             real DEFAULT 0 NOT NULL
);

REVOKE ALL ON ventas FROM PUBLIC;
GRANT SELECT ON ventas to GROUP osopos;
GRANT INSERT,SELECT ON ventas TO "supervisor";

CREATE TABLE ventas_detalle (
  "id_venta"        int4 NOT NULL,
  "codigo"          varchar(20) NOT NULL,
  "descrip"         varchar(40) NOT NULL,
  "cantidad"        real,
  "pu"              float NOT NULL DEFAULT 0,
  "iva_porc"        float not null default 15,
  "tax_0"           real DEFAULT 0 NOT NULL,
  "tax_1"           real DEFAULT 0 NOT NULL,
  "tax_2"           real DEFAULT 0 NOT NULL,
  "tax_3"           real DEFAULT 0 NOT NULL,
  "tax_4"           real DEFAULT 0 NOT NULL,
  "tax_5"           real DEFAULT 0 NOT NULL
);
REVOKE ALL ON ventas_detalle FROM PUBLIC;
GRANT SELECT ON ventas_detalle to GROUP osopos;

CREATE TABLE facturas_ingresos (
  "id"              SERIAL PRIMARY KEY,
  "fecha"           DATE,
  "rfc"             character varying(13),
  "dom_calle"       character varying(30),
  "dom_numero"      character varying(15),
  "dom_inter"       character varying(3),
  "dom_col"         character varying(20),
  "dom_ciudad"      character varying(30),
  "dom_edo"         character varying(20),
  "dom_cp"          int4,
  "subtotal"        real not null,
  "iva"             real not null,
  "tax_0"           real not null default 0,
  "tax_1"           real not null default 0,
  "tax_2"           real not null default 0,
  "tax_3"           real not null default 0,
  "tax_4"           real not null default 0,
  "tax_5"           real not null default 0
);
REVOKE ALL ON facturas_ingresos FROM PUBLIC;
GRANT SELECT ON facturas_ingresos to GROUP osopos;

CREATE TABLE fact_ingresos_detalle (
  id_factura        int4 not null,
  codigo            varchar(20) default '',
  concepto          varchar(128) not null,
  cant              int,
  precio            real
);
REVOKE ALL ON fact_ingresos_detalle FROM PUBLIC;
GRANT SELECT ON fact_ingresos_detalle to GROUP osopos;

CREATE TABLE clientes_fiscales (
  rfc               varchar(13) PRIMARY KEY NOT NULL,
  curp              varchar(18),
  nombre            varchar(70) NOT NULL
);
REVOKE ALL ON clientes_fiscales FROM PUBLIC;
GRANT SELECT ON clientes_fiscales TO GROUP osopos;

CREATE TABLE departamento (
  id                SERIAL PRIMARY KEY,
  nombre            varchar(25)
);
REVOKE ALL ON departamento FROM PUBLIC;
INSERT INTO departamento VALUES (0, 'Sin clasificar');
GRANT SELECT ON departamento TO GROUP osopos;

CREATE TABLE proveedores (
  "id"          SERIAL PRIMARY KEY,
  "nick"        varchar(15) NOT NULL,
  "razon_soc"   varchar(30),
  "calle"       varchar(30),
  "colonia"     varchar(25),
  "ciudad"      varchar(30),
  "estado"      varchar(30),
  "contacto"    varchar(40),
  "email"       varchar(40),
  "url"         varchar(80)
);
REVOKE ALL ON proveedores FROM PUBLIC;
INSERT INTO proveedores (id, nick) VALUES (0, 'Sin proveedor');
GRANT SELECT ON proveedores TO GROUP osopos;

CREATE TABLE telefonos_proveedor (
  "id_proveedor" int4 NOT NULL,
  "clave_ld"     varchar(3) DEFAULT NULL,
  "numero"       varchar(7) NOT NULL,
  "ext"          int2,
  "fax"          bool DEFAULT 'f'
);
REVOKE ALL ON telefonos_proveedor FROM PUBLIC;
GRANT SELECT ON telefonos_proveedores TO GROUP osopos;

CREATE TABLE users (
  "id"           SERIAL NOT NULL,
  "user"         varchar(10) NOT NULL,
  "passwd"       varchar(32) DEFAULT '',
  "level"        int NOT NULL DEFAULT 0,
  "name"         varchar(254)
);
REVOKE ALL ON users FROM PUBLIC;
GRANT SELECT ON users TO osopos_p;

CREATE TABLE divisas (
  "id"           char(3) NOT NULL,
  "nombre"       varchar(20),
  "tipo_cambio"  real
);
REVOKE ALL ON divisas FROM PUBLIC;
GRANT SELECT ON divisas TO GROUP osopos;

CREATE TABLE tipo_mov_inv (
  "id"           SERIAL NOT NULL,
  "nombre"       varchar(40),
  "entrada"      boolean DEFAULT TRUE
);
REVOKE ALL ON tipo_mov_inv FROM PUBLIC;
GRANT SELECT ON tipo_mov_inv TO GROUP osopos;
INSERT INTO tipo_mov_inv (nombre, entrada) VALUES ('Venta', 'F');
INSERT INTO tipo_mov_inv (nombre, entrada) VALUES ('Compra', 'T');
INSERT INTO tipo_mov_inv (nombre, entrada) VALUES ('Devolución de venta', 'T');
INSERT INTO tipo_mov_inv (nombre, entrada) VALUES ('Devolución de compra', 'F');
INSERT INTO tipo_mov_inv (nombre, entrada) VALUES ('Merma', 'F');
INSERT INTO tipo_mov_inv (nombre, entrada) VALUES ('Transferencia (salida)', 'F');
INSERT INTO tipo_mov_inv (nombre, entrada) VALUES ('Transferencia (entrada)', 'T');

CREATE TABLE mov_inv (
  "id"          SERIAL NOT NULL,
  "almacen"     int NOT NULL default 1,
  "tipo_mov"    int NOT NULL,
  "usuario"     varchar(10) NOT NULL,
  "fecha_hora"  timestamp NOT NULL,
  "id_prov1"    int
);
REVOKE ALL ON mov_inv FROM PUBLIC;
GRANT SELECT ON mov_inv TO GROUP osopos;

CREATE TABLE mov_inv_detalle (
  id          int,
  codigo      varchar(20) NOT NULL,
  cant        real NOT NULL,
  pu          real,
  p_costo     real,
  alm_dest    int
);
REVOKE ALL ON mov_inv_detalle FROM PUBLIC;
GRANT SELECT ON mov_inv_detalle TO GROUP osopos;
CREATE INDEX mov_inv_detalle_bkey ON mov_inv_detalle USING BTREE(id);

CREATE TABLE perfiles (
  id          SERIAL,
  nombre      varchar(30) NOT NULL
);
REVOKE ALL ON perfiles FROM PUBLIC;
GRANT SELECT ON perfiles TO GROUP osopos;
GRANT ALL ON perfiles TO "scaja";

CREATE TABLE modulo_perfil (
  perfil       VARCHAR(30),
  nombre       VARCHAR(30)
);
CREATE INDEX modulo_perfil_bkey ON modulo_perfil USING BTREE(perfil);
REVOKE ALL ON modulo_perfil FROM PUBLIC;
GRANT SELECT ON modulo_perfil TO GROUP osopos;
GRANT ALL ON modulo_perfil TO "scaja";

CREATE TABLE modulo (
  "id"           serial,
  "nombre"       varchar(30),
  "desc"         varchar(60)
);
REVOKE ALL ON modulo FROM PUBLIC;
GRANT SELECT ON modulo TO GROUP osopos;

INSERT INTO modulo (nombre, "desc") VALUES ('invent_ver_pcosto', 'Inventarios. Ver precio de costo');
INSERT INTO modulo (nombre, "desc") VALUES ('invent_ver_prov', 'Inventarios. Ver proveedores');
INSERT INTO modulo (nombre, "desc") VALUES ('invent_borrar_item', 'Inventarios. Borrar item');
INSERT INTO modulo (nombre, "desc") VALUES ('invent_cambiar_item', 'Inventarios. Modificar item');
INSERT INTO modulo (nombre, "desc") VALUES ('invent_depto_renombrar', 'Inventarios. Renombrar departamento');
INSERT INTO modulo (nombre, "desc") VALUES ('invent_general', 'Inventarios. Acceso general');
INSERT INTO modulo (nombre, "desc") VALUES ('movinv_general', 'Mov. al inv. Acceso general');
INSERT INTO modulo (nombre, "desc") VALUES ('movinv_compra', 'Mov. al inv. Registrar compras');
INSERT INTO modulo (nombre, "desc") VALUES ('movinv_venta', 'Mov. al inv. Registrar ventas');
INSERT INTO modulo (nombre, "desc") VALUES ('movinv_devventa', 'Mov. al inv. Registrar dev. de ventas');
INSERT INTO modulo (nombre, "desc") VALUES ('movinv_devcompra', 'Mov. al inv. Registrar dev. compras');
INSERT INTO modulo (nombre, "desc") VALUES ('movinv_merma', 'Mov. al inv. Registrar mermas');
INSERT INTO modulo (nombre, "desc") VALUES ('movinv_tsalida', 'Mov. al inv. Registrar transferencia de salida');
INSERT INTO modulo (nombre, "desc") VALUES ('movinv_tentrada', 'Mov. al inv. Registrar transferencia de entrada');
INSERT INTO modulo (nombre, "desc") VALUES ('usuarios_general', 'Admin. de usuarios. Acceso general');
INSERT INTO modulo (nombre, "desc") VALUES ('caja_cajon_manual', 'Operación de caja. Apertura manual de cajón de efectivo');

CREATE TABLE modulo_usuarios (
  id           int NOT NULL DEFAULT 0,
  usuario      VARCHAR(10) NOT NULL
);
REVOKE ALL ON modulo_usuarios FROM PUBLIC;
GRANT SELECT ON modulo_usuarios TO GROUP osopos;
