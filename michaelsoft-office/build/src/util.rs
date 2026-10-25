use rand::seq::IteratorRandom;

pub fn ident(prefix: &str) -> String {
    static ALPHABET: &str = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    let suffix = (0..16)
        .map(|_| ALPHABET.chars().choose(&mut rand::rng()).unwrap())
        .collect::<String>();
    // format!("{}{}", prefix, suffix)
    format!("{}", suffix)
}

pub fn idents(prefix: &str, n: usize) -> Vec<String> {
    (0..n).map(|_| ident(prefix)).collect()
}

pub fn all(conds: impl IntoIterator<Item = String>) -> String {
    conds.into_iter().collect::<Vec<_>>().join(" AND ")
}
