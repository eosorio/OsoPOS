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
  tangible      boolean DEFAULT TRUE,
  granel        boolean DEFAULT FALSE
);
REVOKE ALL ON articulos FROM PUBLIC;
GRANT SELECT ON articulos to GROUP osopos;
GRANT ALL ON articulos TO "scaja";

CREATE TABLE articulos_costos (
  codigo        varchar(20) NOT NULL REFERENCES articulos,
  id_prov       int4 NOT NULL,
  costo1        real NOT NULL,
  costo2        real NOT NULL,
  prov_clave    varchar(20),
  iva_porc      real NOT NULL,
  entrega1      int2,
  entrega2      int2,
  costo_envio1  real,
  costo_envio2  real,
  actualizacion timestamp NOT NULL DEFAULT current_timestamp,
  status        character(1),
  divisa        character(3) NOT NULL DEFAULT 'MXP',
  divisa_envio  character(3) NOT NULL DEFAULT 'MXP'
);
REVOKE ALL ON articulos_costos FROM PUBLIC;
GRANT SELECT ON articulos_costos to GROUP osopos;
GRANT ALL ON articulos_costos TO "scaja";


-- numeros de serie

CREATE TABLE articulos_series (
  id        varchar(50) PRIMARY KEY NOT NULL,
  codigo    varchar(20) REFERENCES articulos,
  almacen   int2
);
REVOKE ALL ON articulos_series FROM PUBLIC;
GRANT SELECT ON articulos_series to GROUP osopos;

-- tipo de propiedades de un producto (talla, color, material, etc

CREATE TABLE articulos_propiedades_tipo (
id             SERIAL PRIMARY KEY NOT NULL,
codigo         varchar(20) REFERENCES articulos NOT NULL,
etiqueta_prop  varchar(30)
);

-- inventarios por propiedades

CREATE TABLE articulos_propiedades_exist (
codigo     varchar(20) REFERENCES articulos NOT NULL,
id_prop1   varchar(40) NOT NULL,
id_prop2   varchar(40),
id_prop3   varchar(40),
id_prop4   varchar(40),
id_prop5   varchar(40),
cant       real NOT NULL DEFAULT 0,
id_almacen int2 NOT NULL DEFAULT 1
);

CREATE TABLE "almacenes" (
  "id"          int2 DEFAULT nextval('almacenes_id_seq'::text) PRIMARY KEY NOT NULL,
  "nombre"      VARCHAR(30),
  "descripcion" VARCHAR(40)
);

CREATE SEQUENCE almacenes_id_seq
    START 1
    INCREMENT 1
    MAXVALUE 1000
    MINVALUE 1
    CACHE 1;
                                                                                
REVOKE ALL ON almacenes FROM PUBLIC;
GRANT SELECT ON almacenes to GROUP osopos;
GRANT ALL ON almacenes TO scaja;

INSERT INTO almacenes VALUES (1, 'Mostrador', 'Mostrador');

CREATE TABLE almacen_1 (
  codigo        varchar(20) NOT NULL REFERENCES articulos,
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
  id_alm        int2 DEFAULT 1 NOT NULL REFERENCES almacenes (id),
  UNIQUE(codigo,id_alm)
);
REVOKE ALL ON almacen_1 FROM PUBLIC;
GRANT SELECT ON almacen_1 to GROUP osopos;
GRANT ALL ON almacen_1 TO "scaja";

CREATE INDEX almacen_1_bkey ON almacen_1 USING BTREE(codigo);

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
  codigo        varchar(20) NOT NULL REFERENCES articulos,
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
GRANT INSERT,SELECT ON corte to GROUP osopos;
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
  "id_venta"        int4 NOT NULL REFERENCES ventas (numero),
  "codigo"          varchar(20) NOT NULL,
  "descrip"         varchar(40) NOT NULL,
  "cantidad"        real NOT NULL,
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
CREATE INDEX ventas_detalle_bkey ON ventas_detalle USING BTREE(id_venta,codigo);
REVOKE ALL ON ventas_detalle FROM PUBLIC;
GRANT SELECT ON ventas_detalle to GROUP osopos;

CREATE TABLE facturas_ingresos (
  "id"              SERIAL PRIMARY KEY,
  "fecha"           DATE,
  "rfc"             character varying(13),
  "dom_calle"       character varying(30),
  "dom_numero"      character varying(15),
  "dom_inter"       character varying(3),
  "dom_col"         character varying(64),
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
  "id"           SERIAL PRIMARY KEY NOT NULL,
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

INSERT INTO divisas VALUES ('MXP', 'Moneda Nacional', 1);

CREATE TABLE tipo_mov_inv (
  "id"           SERIAL PRIMARY KEY NOT NULL,
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
  "id"          SERIAL PRIMARY KEY NOT NULL,
  "almacen"     int NOT NULL default 1,
  "tipo_mov"    int NOT NULL,
  "usuario"     varchar(10) NOT NULL,
  "fecha_hora"  timestamp NOT NULL DEFAULT current_timestamp,
  "id_prov1"    int
);
REVOKE ALL ON mov_inv FROM PUBLIC;
GRANT SELECT ON mov_inv TO GROUP osopos;

CREATE TABLE mov_inv_detalle (
  id          int REFERENCES mov_inv,
  codigo      varchar(20) NOT NULL REFERENCES articulos,
  cant        real NOT NULL,
  pu          real,
  p_costo     real,
  alm_dest    int
);
REVOKE ALL ON mov_inv_detalle FROM PUBLIC;
GRANT SELECT ON mov_inv_detalle TO GROUP osopos;
CREATE INDEX mov_inv_detalle_bkey ON mov_inv_detalle USING BTREE(id);

CREATE TABLE perfiles (
  id          SERIAL PRIMARY KEY,
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
  "id"           SERIAL PRIMARY KEY,
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
  pedido      timestamp NOT NULL DEFAULT current_timestamp,
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
  codigo   varchar(20) NOT NULL REFERENCES articulos,
  dia      int NOT NULL,             -- todos: 0, domingo: 1, lunes: 2, etc.
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
  f_entrega timestamp NOT NULL DEFAULT current_timestamp,      -- Fecha de entrega
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
INSERT INTO status_rentas_detalle (id, nick, descr) VALUES (1, 'Rentado', 'Articulo que se encuentra rentado');

CREATE TABLE cliente (
  id                 serial,
  razon_soc     	 varchar(128),
  ap_paterno    	 varchar(80),
  ap_materno    	 varchar(80),
  nombres       	 varchar(100),
  dom_calle     	 varchar(45),
  dom_numero         varchar(15),
  dom_inter          varchar(10),
  dom_col            varchar(64),
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
  alta               timestamp NOT NULL DEFAULT current_timestamp,
  baja               timestamp,
  status             bit(8)
);

CREATE TABLE cliente_trabajo (
  id          int4 PRIMARY KEY NOT NULL,
  dom_calle   varchar(45),
  dom_inter   varchar(10),
  dom_numero  varchar(15),
  dom_col     varchar(64),
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
GRANT INSERT,SELECT ON folios_tickets_1 TO GROUP osopos;
GRANT UPDATE ON folios_tickets_1_id_seq TO GROUP osopos;

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
  codigo      varchar(20) NOT NULL REFERENCES articulos,
  cant        int2 NOT NULL DEFAULT 1
);
CREATE INDEX carro_virtual_bkey ON carro_virtual USING BTREE(usuario);

-- Catálogo de compras y pedidos a proveedor (y posiblemente a clientes)
CREATE TABLE pedido_p_cabecera (
  id          SERIAL PRIMARY KEY,
  docto       int4 NOT NULL DEFAULT 0,
  id_prov     int2 NOT NULL,
  dom_entrega int4 NOT NULL REFERENCES domicilios,
  subtotal    float NOT NULL,
  descuento   float NOT NULL,
  iva         float NOT NULL DEFAULT 0,
  total       float NOT NULL,
  observaciones varchar(256),
  id_usuario  int2 NOT NULL,
  fecha       timestamp NOT NULL DEFAULT current_timestamp,
  fecha_envio timestamp,
  almacen     int2 NOT NULL DEFAULT 1, --Almacen en el que se recibe ó entrega
  tipo        int2 NOT NULL DEFAULT 1,
  fecha_pedido date NOT NULL,
  tipo_pago   int2 NOT NULL DEFAULT 1 REFERENCES forma_pago(id),
  estatus     char(1)
);

CREATE TABLE pedido_p_detalle(
  id          int4 NOT NULL REFERENCES pedido_p_cabecera,
  codigo      varchar(20) NOT NULL REFERENCES articulos,
  cant        float NOT NULL DEFAULT 1,
  pu          float NOT NULL,
  descuento   float NOT NULL DEFAULT 0,
  iva         float NOT NULL DEFAULT 15,
  guia        varchar(64)
);
CREATE INDEX pedido_p_detalle_bkey ON pedido_p_detalle USING BTREE(id);

CREATE TABLE pedido_c_cabecera (
  id          SERIAL PRIMARY KEY,
  docto       int4 NOT NULL DEFAULT 0,
  id_cte      int2 NOT NULL,
  dom_entrega int4 NOT NULL REFERENCES domicilios,
  subtotal    float NOT NULL,
  descuento   float NOT NULL DEFAULT 0,
  iva         float NOT NULL DEFAULT 0,
  total       float NOT NULL,
  observaciones varchar(256),
  id_usuario  int2 NOT NULL,
  fecha       timestamp NOT NULL DEFAULT current_timestamp,
  fecha_envio timestamp,
  tipo        int2 NOT NULL DEFAULT 1,
  fecha_pedido date NOT NULL,
  tipo_pago   int2 NOT NULL DEFAULT 1 REFERENCES forma_pago (id),
  estatus     char(1)
);

CREATE TABLE pedido_c_detalle(
  id          int4 NOT NULL REFERENCES pedido_c_cabecera,
  codigo      varchar(20) NOT NULL REFERENCES articulos,
  cant        float NOT NULL DEFAULT 1,
  pu          float NOT NULL,
  descuento   float NOT NULL DEFAULT 0,
  iva         float NOT NULL DEFAULT 15,
  guia        varchar(64)
);
CREATE INDEX pedido_c_detalle_bkey ON pedido_c_detalle USING BTREE(id);

CREATE TABLE articulos_garantias (
  codigo   varchar(20) NOT NULL REFERENCES articulos,
  tiempo   int NOT NULL,  -- unidad de tiempo de la garantia
  unidad_t int NOT NULL default 2,
  UNIQUE(codigo)
);
  
CREATE SEQUENCE domicilio_paises_id_seq
    START 1
    INCREMENT 1
    MAXVALUE 2147483647
    MINVALUE 1
    CACHE 1;

CREATE TABLE domicilio_paises (
  id     int2 DEFAULT nextval('domicilio_paises_id_seq'::text) PRIMARY KEY NOT NULL,
  nombre varchar(32) NOT NULL
);
INSERT INTO domicilio_paises (nombre) VALUES ('México');

CREATE TABLE domicilio_estados (
  id     int2 PRIMARY KEY NOT NULL,
  nombre varchar(32) NOT NULL
);

CREATE TABLE domicilios (
  id            SERIAL PRIMARY KEY NOT NULL,
  id_cliente    int4 NOT NULL REFERENCES clientes(id),
  dom_nombre    varchar(32),
  dom_calle     varchar(64),
  dom_numero    varchar(15),
  dom_inter     varchar(25),
  dom_col       varchar(64),
  dom_mpo       varchar(32),
  dom_ciudad    varchar(30) NOT NULL,
  dom_edo_id    int2 REFERENCES domicilio_estados(id),
  dom_cp        int4,
  dom_pais_id   int2 REFERENCES domicilio_paises(id),
  dom_telefono  varchar(32),
  dom_contacto  varchar(32),
  dom_notas     varchar(64)
);

CREATE SEQUENCE cliente_tipo_id_seq
    START 1
    INCREMENT 1
    MAXVALUE 2147483647
    MINVALUE 1
    CACHE 1;
                                                                                

CREATE TABLE cliente_tipo (
  id    int2 DEFAULT nextval('cliente_tipo_id_seq'::text) PRIMARY KEY NOT NULL,
  tipo  varchar(32)
);

CREATE SEQUENCE clientes_id_seq
    START 1
    INCREMENT 1
    MAXVALUE 2147483647
    MINVALUE 1
    CACHE 1;
                                                                                
CREATE TABLE clientes (
  id           int4 DEFAULT nextval('clientes_id_seq'::text) PRIMARY KEY NOT NULL,
  nombres      varchar(64) NOT NULL,
  ap_paterno   varchar(32),
  ap_materno   varchar(32),
  tipo_cliente int2 REFERENCES cliente_tipo (id),
  sexo         char(1),
  nombre_comer varchar(64),
  rfc          varchar(13),
  dom_principal int4,
  email        varchar(128),
  url          varchar(128),
  telefono1    varchar(32),
  telefono2    varchar(32),
  fax          varchar(32),
  contacto     varchar(128),
  observaciones varchar(128)
);

CREATE TABLE arts_lineas (
  id       SERIAL PRIMARY KEY NOT NULL,
  linea    varchar(32)
);

CREATE SEQUENCE arts_propiedades_tipo_id_seq
    START 1
    INCREMENT 1
    MAXVALUE 2147483647
    MINVALUE 1
    CACHE 1;

CREATE TABLE arts_propiedad_tipo (
  id        int2 DEFAULT nextval('arts_propiedades_tipo_id_seq'::text) NOT NULL,
  id_linea  int4 NOT NULL DEFAULT 0,
  nombre    varchar(32) NOT NULL
);
CREATE INDEX arts_propiedad_tipo_bkey ON arts_propiedad_tipo USING BTREE(id_linea,id);

CREATE TABLE arts_propiedad_nombre (
--  id_tipo     int2 NOT NULL REFERENCES arts_propiedad_tipo (id),
  id_tipo     int2 NOT NULL,
  id          int2 NOT NULL,
  propiedad   varchar(32) NOT NULL
);
CREATE INDEX arts_propiedad_nombre_bkey ON arts_propiedad_nombre USING BTREE(id_tipo,id);

CREATE TABLE arts_propiedad_combo (
  id                 int4 NOT NULL,
  id_propiedad_tipo  int2 NOT NULL,
--  id_propiedad       int2 NOT NULL REFERENCES arts_propiedad_nombre (id)
  id_propiedad       int2 NOT NULL
);
CREATE INDEX arts_propiedad_combo_bkey ON arts_propiedad_combo USING BTREE(id);

CREATE TABLE arts_propiedad_exist (
  codigo             varchar(20) NOT NULL REFERENCES articulos,
--  id_combo           int4 NOT NULL REFERENCES arts_propiedad_combo (id),
  id_combo           int4 NOT NULL,
  cant               float NOT NULL
);
CREATE INDEX arts_propiedad_exist_bkey ON arts_propiedad_exist USING BTREE(codigo,id_combo);

CREATE TABLE "configuracion_grupo" (
  "id" int4 NOT NULL,
  "titulo" varchar(64) NOT NULL default '',
  "descripcion" varchar(255) NOT NULL default '',
  "orden" int2 default NULL,
  "visible" int2 default '1',
  PRIMARY KEY  ("id")
);
INSERT INTO configuracion_grupo VALUES (1, 'Directorios', 'Información de la ruta de los directorios que usa OsoPOS', 1, 1);
INSERT INTO configuracion_grupo VALUES (2, 'Impresoras', 'Colas de impresión para emitir los diferentes documentos', 1, 1);

CREATE TABLE configuracion (
  id SERIAL NOT NULL,
  titulo varchar(64) NOT NULL default '',
  llave varchar(64) NOT NULL default '',
  valor varchar(255) NOT NULL default '',
  descripcion varchar(255) NOT NULL default '',
  id_grupo int4 NOT NULL default '0',
  orden int2 default NULL,
  fecha_modificado timestamp default NULL,
  fecha_agregado timestamp NOT NULL default current_timestamp,
  PRIMARY KEY (id)
);

INSERT INTO configuracion (titulo, llave, valor, descripcion, id_grupo) VALUES ('Directorio principal', 'PWD_DIR', '/var/www/html/desarrollo/osopos-web', 'Directorio donde se instaló el sistema OsoPOS', 1);
INSERT INTO configuracion (titulo, llave, valor, descripcion, id_grupo) VALUES ('Dir. temporal', 'TMP_DIR', '/tmp', 'Directorio donde se generan los archivos temporales', 1);
INSERT INTO configuracion (titulo, llave, valor, descripcion, id_grupo) VALUES ('Directorio de imágenes', 'IMG_DIR', 'imagenes/articulos', 'Directorio de imagenes', 1);

INSERT INTO configuracion (titulo, llave, valor, descripcion, id_grupo) VALUES ('Tickets', 'COLA_TICKET', 'ticket', 'Cola de impresión para emisión de tickets', 2);
INSERT INTO configuracion (titulo, llave, valor, descripcion, id_grupo) VALUES ('Notas de venta', 'COLA_NOTA', 'facturas', 'Cola de impresión para emisión de notas de venta', 2);
INSERT INTO configuracion (titulo, llave, valor, descripcion, id_grupo) VALUES ('Facturas', 'COLA_FACTUR', 'facturas', 'Cola de impresión para emisión de facturas', 2);
INSERT INTO configuracion (titulo, llave, valor, descripcion, id_grupo) VALUES ('Etiquetas', 'COLA_ETIQUETA', 'etiquetas', 'Cola de impresión para impresora térmica de etiquetas', 2);
INSERT INTO configuracion (titulo, llave, valor, descripcion, id_grupo) VALUES ('Predeterminada', 'COLA_DEFAULT', 'facturas', 'Cola de impresión predeterminada', 2);
INSERT INTO configuracion (titulo, llave, valor, descripcion, id_grupo) VALUES ('Comando de impresión', 'CMD_IMPRESION', 'lpr -l ', 'Comando de impresión', 2);
INSERT INTO configuracion (titulo, llave, valor, descripcion, id_grupo) VALUES
('Almacén por omisión', 'ALM_DEF', 1, 'Almacén por omisión', 3);

CREATE TABLE configuracion_pos (
  id_pos int4 NOT NULL default '0',
  llave varchar(64) NOT NULL default '',
  titulo varchar(64) NOT NULL default '',
  valor varchar(255) NOT NULL default '',
  fecha_modificado timestamp default NULL default current_timestamp,
  fecha_agregado timestamp NOT NULL default current_timestamp,
  UNIQUE(llave, id_pos)
);
CREATE INDEX configuracion_pos_bkey ON configuracion_pos USING BTREE(id_pos, llave);

INSERT INTO configuracion_pos (id_pos, llave, titulo, valor) VALUES (0, 'MINIIMPRESORA_TIPO', 'Tipo de emulación de miniprinter', 'EPSON');
INSERT INTO configuracion_pos (id_pos, llave, titulo, valor) VALUES (0, 'IMPRESION_COMANDO', 'Comando del sistema para imprimir', 'lpr -P ');
INSERT INTO configuracion_pos (id_pos, llave, titulo, valor) VALUES (0, 'MINIIMPRESORA_AUTOCUTTER', 'Se usará el corte automático?', '0');
INSERT INTO configuracion_pos (id_pos, llave, titulo, valor) VALUES (0, 'ALMACEN', 'Número de almacén del que proviene la mercancía', '1');
INSERT INTO configuracion_pos (id_pos, llave, titulo, valor) VALUES (0, 'SCANNER_PUERTOSERIE', 'En que puerto serial se encuentra el scanner?', '');
INSERT INTO configuracion_pos (id_pos, llave, titulo, valor) VALUES (0, 'DIVISA', 'Divisa por omisión', 'MXP');
INSERT INTO configuracion_pos (id_pos, llave, titulo, valor) VALUES (0, 'LISTAR_PRECIO_NETO', 'Se muestra en pantalla el precio con impuestos incluidos?', '1');
INSERT INTO configuracion_pos (id_pos, llave, titulo, valor) VALUES (0, 'DESGLOSAR_DESCUENTO', '', '0');
INSERT INTO configuracion_pos (id_pos, llave, titulo, valor) VALUES (0, 'CAPTURAR_SERIE', 'Captura de número de serie en lugar de id. de producto?', '0');
INSERT INTO configuracion_pos (id_pos, llave, titulo, valor) VALUES (0, 'CONSULTA_CATALOGO', 'Consulta producto de memoria o de catálogo', '0');
INSERT INTO configuracion_pos (id_pos, llave, titulo, valor) VALUES (0, 'MODO_RENTA', 'En lugar de vender, se renta?', '0');
INSERT INTO configuracion_pos (id_pos, llave, titulo, valor) VALUES (0, 'REGISTRAR_VENDEDOR', 'Se lleva registro de vendedores?', '0');
INSERT INTO configuracion_pos (id_pos, llave, titulo, valor) VALUES (0, 'COLA_TICKET', 'Cola de impresión de tickets', 'ticket');

