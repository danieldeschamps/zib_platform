/*-----------------------------------------------------------------------*/
DROP TABLE t_attributes;
DROP TABLE t_nodes;

/*************************************************************************/

/*-----------------------------------------------------------------------*/
CREATE TABLE t_nodes (
mac_addr BIGINT NOT NULL,
nwk_addr SMALLINT,
profile_id SMALLINT,
device_id SMALLINT,
online SMALLINT,
tag VARCHAR(50),
PRIMARY KEY (mac_addr) );

CREATE TABLE t_attributes (
mac_addr BIGINT NOT NULL,
ep_number SMALLINT NOT NULL,
cluster_id SMALLINT NOT NULL,
attribute_id SMALLINT NOT NULL,
attribute_value VARCHAR(50),
up_limit VARCHAR(50),
down_limit VARCHAR(50),
limit_action SMALLINT,
FOREIGN KEY (mac_addr) REFERENCES t_nodes (mac_addr) );

CREATE TABLE t_attributes_history (
mac_addr BIGINT NOT NULL,
ep_number SMALLINT NOT NULL,
cluster_id SMALLINT NOT NULL,
attribute_id SMALLINT NOT NULL,
measure_time TIMESTAMP NOT NULL,
attribute_value VARCHAR(50),
attribute_value_double DOUBLE PRECISION,
FOREIGN KEY (mac_addr) REFERENCES t_nodes (mac_addr) );

/*************************************************************************/

/*-----------------------------------------------------------------------*/
INSERT INTO t_nodes	(mac_addr, nwk_addr, profile_id, device_id, online)
	VALUES		(72623859790382856, 0, 256, -6, 1);
INSERT INTO t_nodes	(mac_addr, nwk_addr, profile_id, device_id, online)
	VALUES		(72623859790382857, 10, 256, -6, 0);

INSERT INTO t_attributes (mac_addr, ep_number, cluster_id, attribute_id, attribute_value)
	VALUES		(72623859790382856, 8, 13, 0, 'FF');
INSERT INTO t_attributes (mac_addr, ep_number, cluster_id, attribute_id, attribute_value)
	VALUES		(72623859790382857, 8, 13, 0, '00');