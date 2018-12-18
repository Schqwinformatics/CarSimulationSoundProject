using UnityEngine.Audio;
using System;
using UnityEngine;

public class AudioManager : MonoBehaviour {

    public SimSound[] sounds;

    void Awake()
    {
        foreach(SimSound s in sounds)
        {
            s.source = gameObject.AddComponent<AudioSource>();
            s.source.clip = s.clip;
            s.source.volume = s.volume;
            s.source.pitch = (float)s.pitch;
            s.source.loop = true;
            s.source.Play();
        }
    }

    public void Play(string name)
    {
        SimSound s = Array.Find(sounds, sound => sound.name == name);
        s.source.Play();
    }

    public void SetPitch(string name, double pitch)
    {
        SimSound s = Array.Find(sounds, sound => sound.name == name);
        s.source.pitch = (float)pitch;
    }

    void Update()
    {
        
    }

}
