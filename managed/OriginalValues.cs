using System;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.CompilerServices;

namespace Zb2Menu {
    // Owns managed references while a modification is active; never stores heap addresses.
    public sealed class OriginalValues {
        struct Key : IEquatable<Key> {
            public object Target;
            public string Name;
            public bool Equals(Key other) { return ReferenceEquals(Target, other.Target) && Name == other.Name; }
            public override bool Equals(object other) { return other is Key && Equals((Key)other); }
            public override int GetHashCode() { return RuntimeHelpers.GetHashCode(Target) ^ Name.GetHashCode(); }
        }
        sealed class Entry {
            public object Original;
            public Action<object> Write;
            public long Seen;
        }
        readonly Dictionary<Key, Entry> entries = new Dictionary<Key, Entry>();
        readonly List<Key> expired = new List<Key>();
        long generation;
        public int Count { get { return entries.Count; } }
        public void Begin() { ++generation; }
        public T ReadOriginal<T>(object target,string name,Func<T> read) {
            Entry entry;
            return entries.TryGetValue(new Key{Target=target,Name=name},out entry) ? (T)entry.Original : read();
        }
        public void Apply<T>(object target, string name, Func<T> read, Action<T> write, Func<T,T> change) {
            var key = new Key {Target=target, Name=name};
            Entry entry;
            if (!entries.TryGetValue(key, out entry)) {
                entry = new Entry {Original=read(), Write=value => write((T)value)};
                entries.Add(key, entry); // Capture must succeed before any mutation.
            }
            entry.Seen = generation;
            write(change((T)entry.Original));
        }
        public void End() {
            expired.Clear();
            Exception failure = null;
            foreach (var pair in entries) {
                if (pair.Value.Seen == generation) continue;
                try { pair.Value.Write(pair.Value.Original); expired.Add(pair.Key); }
                catch (Exception ex) { failure = ex; } // Retain originals for retry.
            }
            foreach (var key in expired) entries.Remove(key);
            if (failure != null) throw new InvalidOperationException("Restauracao pendente", failure);
        }
        public void RestoreAll() { Begin(); End(); }
    }
}
