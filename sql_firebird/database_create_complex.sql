/*-----------------------------------------------------------------------*/
ALTER TABLE t_devices DROP CONSTRAINT fk_profile_id_for_device;
ALTER TABLE t_nodes DROP CONSTRAINT fk_profile_id_for_node;
ALTER TABLE t_nodes DROP CONSTRAINT fk_device_id;

DROP TABLE t_profiles;
DROP TABLE t_devices;
DROP TABLE t_nodes;
DROP GENERATOR cod_node_gen;

/*************************************************************************/

/*-----------------------------------------------------------------------*/
CREATE TABLE t_profiles (
profile_id SMALLINT NOT NULL,
descr_profile VARCHAR(30),
PRIMARY KEY (profile_id)
);

/*-----------------------------------------------------------------------*/
CREATE TABLE t_devices (
device_id SMALLINT NOT NULL,
profile_id SMALLINT NOT NULL,
descr_device VARCHAR(30),
PRIMARY KEY (device_id)
);

/*-----------------------------------------------------------------------*/
CREATE TABLE t_nodes (
cod_node INTEGER NOT NULL,
mac_addr BIGINT NOT NULL CONSTRAINT c_mac_addr UNIQUE,
nwk_addr SMALLINT,
profile_id SMALLINT,
device_id SMALLINT,
online SMALLINT,
PRIMARY KEY (cod_node)
);

CREATE GENERATOR cod_node_gen;

SET TERM ^ ;
CREATE TRIGGER "trig_cod_node" FOR "T_NODES" 
ACTIVE BEFORE INSERT POSITION 0
as
begin
  IF (new.cod_node IS NULL) then
    begin
      new.cod_node = gen_id( cod_node_gen, 1 );
    end
end
 ^
COMMIT WORK ^
SET TERM ;^

/*-----------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
ALTER TABLE t_devices ADD CONSTRAINT fk_profile_id_for_device FOREIGN KEY (profile_id) REFERENCES t_profiles (profile_id);
ALTER TABLE t_nodes ADD CONSTRAINT fk_profile_id_for_node FOREIGN KEY (profile_id) REFERENCES t_profiles (profile_id);
ALTER TABLE t_nodes ADD CONSTRAINT fk_device_id FOREIGN KEY (device_id) REFERENCES t_devices (device_id);

/*************************************************************************/

/*-----------------------------------------------------------------------*/
INSERT INTO t_profiles	(profile_id, descr_profile)
	VALUES		(256, 'Home Control, Lighting');
INSERT INTO t_profiles	(profile_id, descr_profile)
	VALUES		(260, 'ZiB Platform');
INSERT INTO t_devices	(device_id, profile_id, descr_device)
	VALUES		(-6, 256, 'Dimming Load Controller');
INSERT INTO t_devices	(device_id, profile_id, descr_device)
	VALUES		(-1, 260, 'Temperature Sensor');
INSERT INTO t_devices	(device_id, profile_id, descr_device)
	VALUES		(-2, 260, 'Humidity Sensor');
INSERT INTO t_devices	(device_id, profile_id, descr_device)
	VALUES		(-3, 260, 'Combo Sensor');
INSERT INTO t_nodes	(mac_addr, nwk_addr, profile_id, device_id, online)
	VALUES		(72623859790382856, 0000, 256, -6, 0);