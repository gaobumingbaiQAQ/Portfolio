using UnityEngine;

public class AutoDoor : MonoBehaviour
{
    private Animator anim;

    // 游戏开始时，找到门的“大脑”
    void Start()
    {
        anim = GetComponent<Animator>();
    }

    // 当有东西走进“感应区”
    void OnTriggerEnter(Collider other)
    {
        // 如果进来的是玩家 (Tag是 Player)
        if (other.CompareTag("Player"))
        {
            anim.SetBool("character_nearby", true); // 打勾，开门
        }
    }

    // 当有东西离开“感应区”
    void OnTriggerExit(Collider other)
    {
        // 如果离开的是玩家
        if (other.CompareTag("Player"))
        {
            anim.SetBool("character_nearby", false); // 去掉勾，关门
        }
    }
}