CREATE TABLE articulos (
  codigo        varchar(20) PRIMARY KEY NOT NULL,
  descripcion   varchar(50) NOT NULL,
  empaque       real,
  id_prov1      int4 DEFAULT 0,
  id_prov2      int4 DEFAULT 0,
  id_depto      int4 DEFAULT 0,
  p_costo       real DEFAULT 0,
  prov_clave    varchar(20) DEFAULT '',
  iva_porc      int DEFAULT 15 NOT NULL,
  divisa        character(3) NOT NULL DEFAULT 'MXP',
  codigo2       varchar(20),
  tax_0         real DEFAULT 5,
  tax_1         real DEFAULT 0,
  tax_2         real DEFAULT 0,
  tax_3         real DEFAULT 0,
  tax_4         real DEFAULT 0,
  tax_5         real DEFAULT 0,
  serie         boolean DEFAULT FALSE,
  tangible      boolean DEFAULT TRUE
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
  actualizacion timestamp,
  status        character(1),
  divisa        character(3) NOT NULL DEFAULT 'MXP',
  divisa_envio  character(3) NOT NULL DEFAULT 'MXP'
);
REVOKE ALL ON articulos_costos FROM PUBLIC;
GRANT SELECT ON articulos_costos to GROUP osopos;
GRANT ALL ON articulos_costos TO "scaja";
  

);

CREATE TABLE articulos_series (
  id        varchar(50) PRIMARY KEY NOT NULL,
  codigo    varchar(20),
  almacen   int2
);
REVOKE ALL ON articulos_series FROM PUBLIC;
GRANT SELECT ON articulos_series to GROUP osopos;

CREATE TABLE almacen_1 (
  codigo        varchar(20) NOT NULL,
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
  tax_5         real DEFAULT 0,
  alquiler      boolean DEFAULT FALSE,   -- El producto solo se alquila
  id_alm        int2 DEFAULT 1 NOT NULL,
  UNIQUE(codigo,id_alm)
);
REVOKE ALL ON almacen_1 FROM PUBLIC;
GRANT SELECT ON almacen_1 to GROUP osopos;
GRANT ALL ON almacen_1 TO "scaja";

CREATE INDEX almacen_1_bkey ON almacen_1 USING BTREE(codigo);

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

-- Catálogo para el carrito de compras

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
  bandera       BIT(8) DEFAULT B'00000000' NOT NULL
);
REVOKE ALL ON corte FROM PUBLIC;
GRANT SELECT ON corte to GROUP osopos;
GRANT INSERT,SELECT,UPDATE ON corte TO "supervisor";

CREATE TABLE forma_pago (
  id           int2 PRIMARY KEY,
  descripcion  varchar(20)
);

INSERT INTO forma_pago VALUES ( 1, 'Tarjeta');
INSERT INTO forma_pago VALUES ( 2, 'Crédito');
INSERT INTO forma_pago VALUES (20, 'Efectivo');
INSERT INTO forma_pago VALUES (21, 'Cheque');
REVOKE ALL ON forma_pago FROM PUBLIC;
GRANT SELECT ON forma_pago to GROUP osopos;
GRANT INSERT,SELECT,UPDATE ON forma_pago TO "supervisor";

CREATE TABLE ventas (
  numero            SERIAL PRIMARY KEY,
  monto             real,
  tipo_pago         int2 DEFAULT 20 NOT NULL,
  tipo_factur       int2 DEFAULT 5 NOT NULL,
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
  "tax_5"           real DEFAULT 0 NOT NULL,
  "util"            real DEFAULT 0 NOT NULL
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
  "razon_soc"   varchar(128),
  "calle"       varchar(128),
  "colonia"     varchar(128),
  "ciudad"      varchar(64),
  "estado"      varchar(64),
  "contacto"    varchar(64),
  "email"       varchar(64),
  "url"         varchar(80),
  "cp"          int4,
  "rfc"         varchar(13)
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
  "nombre"       varchar(30) NOT NULL,
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

CREATE TABLE rentas (
  id          SERIAL,
  pedido      timestamp NOT NULL,
  cliente     int4 NOT NULL,
  status      bit(8) DEFAULT B'00000000' NOT NULL
);
REVOKE ALL ON rentas FROM PUBLIC;

CREATE TABLE status_rentas (
  id       int2 PRIMARY KEY,
  nick     varchar(40) NOT NULL,
  descr    varchar(80)
);


-- ALTER TABLE articulos_rentas RENAME TO articulos_rentas_old;

-- ALTER TABLE articulos_rentas_old DROP CONSTRAINT articulos_rentas_pkey;

-- INSERT INTO articulos_rentas (codigo, dia, pu1, pu2, pu3, pu4, pu5, tiempo, unidad_t) SELECT codigo, 0, p0_1, p0_2, p0_3, p0_4, p0_5, tiempo0, unidad_t from articulos_rentas_old ;

CREATE TABLE articulos_rentas (
  codigo   varchar(20) NOT NULL,
  dia      int NOT NULL,             -- domingo: 0, lunes: 1, etc.
  pu1      float not null default 0,
  pu2      float not null default 0,
  pu3      float not null default 0,
  pu4      float not null default 0,
  pu5      float not null default 0,
  tiempo   float not null default 1, -- tiempo otorgado de renta, en unidades unidad_t
  unidad_t int NOT NULL default 2,
  -- unidad_t: 0 = minutos, 1 = horas, 2 = días, 3 = semanas, 4 = meses, 5 = años
  UNIQUE(codigo, dia)
);
REVOKE ALL ON articulos_rentas FROM PUBLIC;



CREATE TABLE rentas_detalle (
  id        int4 NOT NULL,           -- Número de renta
  serie     varchar(20) NOT NULL,    -- serie del producto
  f_entrega timestamp NOT NULL,      -- Fecha de entrega
  costo     float,
  status    bit(8) DEFAULT B'00000000' NOT NULL,
  entregado   timestamp
);

-- Catálogo que define el significado de los bits "status"
-- en el catálogo artículos_rentados
CREATE TABLE status_rentas_detalle (
  id       int2 PRIMARY KEY,
  nick     varchar(40),
  descr    varchar(80)
);
INSERT INTO status_rentados (id, nick, descr) VALUES (1, 'Rentado', 'Articulo que se encuentra rentado');

CREATE TABLE cliente (
  id                 serial,
  razon_soc     	 varchar(128),
  ap_paterno    	 varchar(80),
  ap_materno    	 varchar(80),
  nombres       	 varchar(100),
  dom_calle     	 varchar(45),
  dom_numero         varchar(15),
  dom_inter          varchar(10),
  dom_col            varchar(20),
  dom_ciudad         varchar(30),
  dom_edo            varchar(20),
  dom_cp             int4,
  dom_tel_casa       varchar(20),
  dom_tel_trabajo    varchar(20),
  referencia1        varchar(120),
  referencia2        varchar(120),
  relacion_ref1      varchar(40),
  relacion_ref2      varchar(40),
  dom_tel_ref1       varchar(20),
  dom_tel_ref2       varchar(20),
  f_nam              date,
  sexo               char(1) NOT NULL,
  ocupacion          varchar(40),
  edo_civil          varchar(40),
  alta               timestamp NOT NULL,
  baja               timestamp,
  status             bit(8)
);

CREATE TABLE cliente_trabajo (
  id          int2 PRIMARY KEY NOT NULL,
  dom_calle   varchar(45),
  dom_inter   varchar(10),
  dom_numero  varchar(15),
  dom_col     varchar(20),
  dom_ciudad  varchar(30),
  dom_edo     varchar(20),
  dom_cp      int4,
  dom_telefono varchar(20),
  dom_tel_ext int2
);

-- Catálogo que define el significado de los bits "status"
-- en el catálogo cliente
CREATE TABLE status_cliente (
  id       int2 PRIMARY KEY,
  nick     varchar(40),
  descr    varchar(80)
);
INSERT INTO status_cliente (id, nick, descr) VALUES (1, 'Renta', 'Cuenta con artículos rentados sin devolver');

CREATE TABLE recup_costo (
  id          varchar(20),      -- Serie del producto
  costo       float NOT NULL,
  ingreso     float
);

CREATE TABLE folios_tickets_1 (
  id          SERIAL,
  venta       int4
);

CREATE TABLE folios_facturas_1 (
  id          SERIAL,
  venta       int4
);

CREATE TABLE folios_notas_1 (
  id          SERIAL,
  venta       int4
);


CREATE TABLE carro_virtual (
  usuario     varchar(10) NOT NULL,
  codigo      varchar(20) NOT NULL,
  cant        int2 NOT NULL DEFAULT 1
);
CREATE INDEX carro_virtual_bkey ON carro_virtual USING BTREE(usuario);

-- Catálogo de compras y pedidos a proveedor (y posiblemente a clientes)
CREATE TABLE pedido_cabecera (
  id          SERIAL,
  folio       int4 NOT NULL,
  id_prov     int2 NOT NULL,
  dom_entrega VARCHAR(256),
  subtotal    float NOT NULL,
  descuento   float NOT NULL,
  iva         float NOT NULL,
  total       float NOT NULL,
  observaciones varchar(256),
  usuario     varchar(20) NOT NULL,
  fecha       timestamp NOT NULL,
  almacen     int2 NOT NULL DEFAULT 1,
  tipo        int2 NOT NULL DEFAULT 1,
  fecha_pedido date NOT NULL
);

CREATE TABLE pedido_detalle(
  id          int4 NOT NULL,
  codigo	  varchar(20) NOT NULL,
  cant        float NOT NULL,
  pu          float NOT NULL,
  descuento   float NOT NULL
);
