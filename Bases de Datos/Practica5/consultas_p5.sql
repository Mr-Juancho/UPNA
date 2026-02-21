--mostrar tablas
SELECT table_name TABLA FROM user_tables;

--mostrar restricciones
SELECT table_name TABLA, constraint_type TIPO, constraint_name RESTRICION
FROM user_constraints;

--desabilitamos la restriccion del departamento--
ALTER TABLE empleado MODIFY CONSTRAINT FK_emp_cod_dep DISABLE;

--insertamos los empleados--
INSERT INTO EMPLEADO VALUES('0001',NULL,'PEDRO','PEREZ','16/10/1970','V',10000,'100');
INSERT INTO EMPLEADO VALUES('1012',NULL,'TERESA','ROMA','21/12/1962','M',22550,'400');

--insertamos departamentos--
INSERT INTO DEPARTAMENTO VALUES ('100','RECURSOS','0001','20/12/1991');
INSERT INTO DEPARTAMENTO VALUES ('400','CONTABILIDAD','0004','30/12/2001');

--la activamos--
ALTER TABLE empleado MODIFY CONSTRAINT FK_emp_cod_dep ENABLE;


--3--
--primero insertamos a pedro perez de nuevo pero cambiendo el cod_emp utilizando una consulta--
--sabemos q su cod_emp es el 0001--
SELECT cod_emp
FROM empleado
WHERE nombre='PEDRO' AND apellido = 'PEREZ';

INSERT INTO empleado VALUES ('0013',
	
    (SELECT supervisor
	FROM empleado
	WHERE nombre='PEDRO' AND apellido = 'PEREZ'),
    
	(SELECT nombre
	FROM empleado
	WHERE nombre='PEDRO' AND apellido = 'PEREZ'),
    
	(SELECT apellido
	FROM empleado
	WHERE nombre='PEDRO' AND apellido = 'PEREZ'),
    
	(SELECT fecha_ncto
	FROM empleado
	WHERE nombre='PEDRO' AND apellido = 'PEREZ'),
    
	(SELECT genero --sexo estaba mal, corregido
	FROM empleado
	WHERE nombre='PEDRO' AND apellido = 'PEREZ'),
    
	(SELECT salario
	FROM empleado
	WHERE nombre='PEDRO' AND apellido = 'PEREZ'),
    
	(SELECT cod_dep
	FROM empleado
	WHERE nombre='PEDRO' AND apellido = 'PEREZ'));
	
UPDATE empleado SET supervisor = '0013'
WHERE supervisor = '0001';

UPDATE departamento SET jefe = '0013'
WHERE jefe = '0001';

UPDATE trabaja SET cod_emp = '0013'
WHERE cod_emp = '0001';

UPDATE dependiente SET cod_emp = '0013'
WHERE cod_emp = '0001';

DELETE empleado
WHERE nombre = 'PEDRO' AND apellido = 'PEREZ' AND cod_emp<>'0013';

-- Se podria realizar tambien: desactivando las foreing key de las tablas empleado,departamento,trabaja y dependiente
-- que hacen referencia al codigo del empleado.  Una vez activadas habria que relaizar todos los update sobre estas mismas tablas. 
-- Una vez hechos los update se eliminaria el resgistro de Pedro Perez. Y por ultimo se volverian a activar 
-- todas restricciones de foreing key previamnete deshabilitadas

--9-Debe reflejarse en la base de datos la siguiente hipótesis semántica:
--“Un local podrá tener Aire Acondicionado o no tenerlo. Inicialmente ninguno lo tiene. Se ha
--dotado de Aire Acondicionado a los departamentos en los cuales el número de horas
--totales trabajadas en proyectos es mayor de 700. Se consideran los departamentos a los
--que pertenecen los empleados, NO los departamentos vinculados a cada proyecto.”
ALTER TABLE departamento ADD (aire VARCHAR2(2) DEFAULT 'NO');
ALTER TABLE departamento ADD (CONSTRAINT CH_dep_aire CHECK (aire IN('SI','NO')));

UPDATE departamento SET aire='SI'
WHERE cod_dep IN (SELECT D.cod_dep
					FROM departamento D, trabaja T, empleado E
					WHERE D.cod_dep=E.cod_dep AND T.cod_emp=E.cod_emp
					GROUP BY D.cod_dep
					HAVING sum(horas)>700);

--11-- 11. Asegura la integridad referencial entre los proyectos y los lugares en los que se desarrollan.
ALTER TABLE PROYECTO
ADD (CONSTRAINT FK_PROY_LOC FOREIGN KEY (COD_DEP,LOCALIZA) REFERENCES ESPACIOS(COD_DEP,LOCALIZA));

--12. El número de horas que un empleado trabaja en un proyecto no puede superar las 180.
ALTER TABLE TRABAJA ADD CONSTRAINT CH_NUMHORASMAX CHECK (HORAS<=180) NOVALIDATE;

--14-- Nos interesa crear una tabla con los datos de los empleados que son supervisores, 
--su NSS,  nombre, apellido y salario. La nueva tabla se llamara¡ Supervisores.   
CREATE TABLE SUPERVISORES(
	NSS VARCHAR2(15),
	NOMBRE VARCHAR2 (15),
	APELLIDO VARCHAR2(15),
	SALARIO NUMBER,
CONSTRAINT PK_SUPERVISORES PRIMARY KEY (NSS));
       
--15-- Crear un nuevo campo en la tabla empleado que tomara¡ los valores J, S o E 
--(Junior, Senior,  Experto), dependiendo de la edad de cada empleado. 
-- El campo no podrÃ¡ tomar valores  nulos. Junior son aquellos menores de 30 años.
--Entre los 31 y los 50 los clasificaremos como  empleados Senior,
-- mientras que los mayores de 50 serÃ¡n expertos.   
ALTER TABLE EMPLEADO ADD NIVEL CHAR(1);
ALTER TABLE EMPLEADO ADD CONSTRAINT CH_NIVELEMP_CODELIST CHECK (NIVEL IN ('J','S','E'));
-- actualizacion de niveles en abse a los años
UPDATE EMPLEADO
SET NIVEL =
CASE 
    WHEN (SYSDATE-FECHA_NCTO)/365.25 <= 30 THEN 'J' 
    WHEN (SYSDATE-FECHA_NCTO)/365.25 between 31 and 50 THEN 'S'
    ELSE 'E' 
END;
-- Añado la rstriccion de no nulo
ALTER TABLE EMPLEADO MODIFY NIVEL CHAR(1) NOT NULL;

select * from EMPLEADO order by FECHA_NCTO;


