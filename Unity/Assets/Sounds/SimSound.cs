using UnityEngine.Audio;
using UnityEngine;

[System.Serializable]
public class SimSound {

    public AudioClip clip;

    public string name;

    [Range(0.001f, 3.00f)]
    public double pitch;
    [Range(0, 1)]
    public float volume;

    [HideInInspector]
    public AudioSource source;
}
