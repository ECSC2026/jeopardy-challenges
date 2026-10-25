use std::io::{self, BufRead, Read, Write};

pub(crate) trait TableRow {
    const NAME: &'static str;
    const COLUMNS: &'static str;
    const TYPES: &'static str;
    const META: &'static str;

    fn read_row(reader: &mut impl BufRead) -> io::Result<Self>
    where
        Self: Sized;
    fn write_row(&self, writer: &mut impl Write) -> io::Result<()>;
}

/// Write all rows of a table to the given writer.
pub fn write_rows<T: TableRow>(writer: &mut impl Write, rows: &[T]) -> io::Result<()> {
    write!(writer, "{}\r\n", T::COLUMNS)?;
    write!(writer, "{}\r\n", T::TYPES)?;
    write!(writer, "{}\r\n", T::META)?;
    for row in rows {
        row.write_row(writer)?;
    }
    Ok(())
}

/// Read all rows of a table from the given reader.
pub fn read_rows<T: TableRow>(reader: &mut impl Read) -> io::Result<Vec<T>> {
    let mut reader = io::BufReader::new(reader);

    let mut str = String::new();
    reader.read_line(&mut str)?;
    if str.trim() != T::COLUMNS {
        return Err(io::Error::new(
            io::ErrorKind::InvalidData,
            format!(
                "expected columns for table {}: {}, got: {}",
                T::NAME,
                T::COLUMNS,
                str.trim()
            ),
        ));
    }

    str.clear();
    reader.read_line(&mut str)?;
    if str.trim() != T::TYPES {
        return Err(io::Error::new(
            io::ErrorKind::InvalidData,
            format!(
                "expected types for table {}: {}, got: {}",
                T::NAME,
                T::TYPES,
                str.trim()
            ),
        ));
    }

    str.clear();
    reader.read_line(&mut str)?;
    if str.trim() != T::META {
        return Err(io::Error::new(
            io::ErrorKind::InvalidData,
            format!(
                "expected meta for table {}: {}, got: {}",
                T::NAME,
                T::META,
                str.trim()
            ),
        ));
    }

    let mut rows = Vec::new();
    while let Ok(row) = T::read_row(&mut reader) {
        rows.push(row);

        if reader.fill_buf()?.is_empty() {
            break;
        }
    }
    Ok(rows)
}

pub(crate) fn escape(s: String) -> String {
    s.replace('\0', "\x15")
        .replace('\x08', "\x1B")
        .replace('\t', "\x10")
        .replace('\n', "\x19")
        .replace('\x0C', "\x18")
        .replace('\r', "\x11")
}

pub(crate) fn unescape(s: String) -> String {
    s.replace("\x15", "\0")
        .replace("\x1B", "\x08")
        .replace("\x10", "\t")
        .replace("\x19", "\n")
        .replace("\x18", "\x0C")
        .replace("\x11", "\r")
}
