CREATE TABLE articulos (
  codigo        varchar(20) PRIMARY KEY NOT NULL,
  descripcion   varchar(50) NOT NULL,
  pu            real,
  descuento     real DEFAULT 0,
  cant          int,
  min           int DEFAULT 0,
  max           int,
  id_prov       int4 DEFAULT 0,
  id_depto      int4 DEFAULT 0,
  p_costo       real DEFAULT 0,
  prov_clave	varchar(20) DEFAULT '',
  iva_porc      int DEFAULT 15 NOT NULL
);
REVOKE ALL ON articulos FROM PUBLIC;
GRANT SELECT,UPDATE ON articulos TO "caja1";
GRANT SELECT,UPDATE ON articulos TO "caja2";
GRANT SELECT,UPDATE ON articulos TO "caja3";
GRANT ALL ON articulos TO "scaja";

CREATE TABLE ventas (
  numero            SERIAL PRIMARY KEY,
  monto             real,
  tipo_pago         int2 NOT NULL DEFAULT 20,
  tipo_factur       int2 NOT NULL DEFAULT 5,
  corte_parcial     bool DEFAULT 'f',
  utilidad          real,
  id_vendedor       int4 NOT NULL DEFAULT 0,
  id_cajero         int4 NOT NULL DEFAULT 0,
  fecha             date NOT NULL,
  hora              time NOT NULL
);

REVOKE ALL ON ventas FROM PUBLIC;
GRANT INSERT,SELECT ON ventas TO "caja1";
GRANT INSERT,SELECT,UPDATE,DELETE ON ventas TO "scaja";

CREATE TABLE ventas_detalle (
  "id_venta"        int4 NOT NULL,
  "codigo"          varchar(20) NOT NULL,
  "descrip"         varchar(40) NOT NULL,
  "cantidad"        int2,
  "pu"              float NOT NULL DEFAULT 0,
  "iva_porc"        float not null default 15
);

CREATE TABLE facturas_ingresos (
  id                SERIAL PRIMARY KEY,
  fecha             DATE,
  rfc               VARCHAR(13),
  dom_calle         VARCHAR(30),
  dom_numero        VARCHAR(15),
  dom_inter         VARCHAR(3),
  dom_col           VARCHAR(20),
  dom_ciudad        VARCHAR(30),
  dom_edo           VARCHAR(3),
  dom_cp            int4,
  subtotal          real not null,
  iva               real
);

CREATE TABLE fact_ingresos_detalle (
  id_factura        int4 not null,
  codigo            varchar(20) default '',
  concepto          varchar(128) not null,
  cant              int,
  precio            real
);

CREATE TABLE clientes_fiscales (
  rfc               varchar(13) PRIMARY KEY NOT NULL,
  curp              varchar(18),
  nombre            varchar(70) NOT NULL
);

CREATE TABLE departamento (
  id                SERIAL PRIMARY KEY,
  nombre            varchar(25)
);
INSERT INTO departamento VALUES (0, 'Sin clasificar');

CREATE TABLE proveedores (
  "id"          SERIAL PRIMARY KEY,
  "nick"        varchar(15) NOT NULL,
  "razon_soc"   varchar(30),
  "calle"       varchar(30),
  "colonia"     varchar(25),
  "ciudad"      varchar(30),
  "estado"      varchar(30),
  "contacto"    varchar(40),
  "email"	varchar(40),
  "url"         varchar(80)
);
INSERT INTO proveedores (id, nick) VALUES (0, 'Sin proveedor');

CREATE TABLE telefonos_proveedor (
  "id_proveedor" int4 NOT NULL,
  "clave_ld"     varchar(3) DEFAULT NULL,
  "numero"       varchar(7) NOT NULL,
  "ext"          int2,
  "fax"          bool DEFAULT 'f'
);

CREATE TABLE password (
  "usuario"      varchar(10),
  "passwd"       varchar(32) DEFAULT ''
);
