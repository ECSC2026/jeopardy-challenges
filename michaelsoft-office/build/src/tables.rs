use std::path::{Path, PathBuf};

use anymap3::AnyMap;

use crate::rw::TableRow;

pub struct TableManager {
    root: PathBuf,

    rows: AnyMap,
    commits: Vec<Box<dyn FnOnce(&mut Self, &Path) -> std::io::Result<()>>>,
}

impl TableManager {
    pub fn new(root: PathBuf) -> Self {
        TableManager {
            root,
            rows: AnyMap::new(),
            commits: Vec::new(),
        }
    }

    pub fn write(mut self, out_dir: &Path) -> std::io::Result<()> {
        let commits = std::mem::take(&mut self.commits);
        for commit in commits {
            commit(&mut self, out_dir)?;
        }
        Ok(())
    }

    pub fn insert<T: 'static + TableRow>(&mut self, row: T) {
        self.table_rows::<T>().push(row);
    }

    pub fn rows_read<T: 'static + TableRow>(&self) -> Option<&Vec<T>> {
        self.rows.get::<Vec<T>>()
    }

    pub fn table_rows<T: 'static + TableRow>(&mut self) -> &mut Vec<T> {
        if !self.rows.contains::<Vec<T>>() {
            let truncated_name = &T::NAME[..T::NAME.len().min(8)];
            let path = self.root.join(format!("{}.idt", truncated_name));
            let mut file = std::fs::File::open(path).expect("Failed to open file");
            let rows = crate::rw::read_rows::<T>(&mut file).expect("Failed to read rows");
            self.rows.insert(rows);
            self.commits
                .push(Box::new(move |manager: &mut Self, out_dir: &Path| {
                    let rows = manager.rows.get_mut::<Vec<T>>().unwrap();
                    let path = out_dir.join(format!("{}.idt", truncated_name));
                    let mut file = std::fs::File::create(path)?;
                    crate::rw::write_rows(&mut file, rows.as_ref())?;
                    Ok(())
                }));
        }
        self.rows.get_mut::<Vec<T>>().unwrap()
    }
}
