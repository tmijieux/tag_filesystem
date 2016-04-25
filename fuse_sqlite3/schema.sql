create table tag
(
	t_id		integer primary key autoincrement,
	t_name		text unique not null on conflict ignore
);

create table file
(
	f_id		integer primary key autoincrement,
	f_name		text unique not null on conflict ignore
);

create table tag_file
(
	t_id		integer,
	f_id		integer,
	foreign key(t_id) references tag(t_id),
	foreign key(f_id) references file(f_id),
	primary key (t_id, f_id)
);
